/******************************************************************************
*                                                                             *
*   Module:     Vga.c                                                         *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       5/9/2000                                                      *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the code for the simple implementation of
        virtual VGA subsystem.

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 5/9/2000   Original                                             Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Include Files                                                             *
******************************************************************************/
#include <strmini.h>                // Include driver level C functions

#include <ntddk.h>                  // Include ddk function header file

#include "Vm.h"                     // Include root header file

/******************************************************************************
*                                                                             *
*   Global Variables                                                          *
*                                                                             *
******************************************************************************/

/******************************************************************************
*                                                                             *
*   Local Defines, Variables and Macros                                       *
*                                                                             *
******************************************************************************/

typedef struct
{
    BYTE MiscOutput;                // w=3C2, r=3CC
    BYTE FeatureControl;            // w=3DA, r=3CA

    BYTE Sequencer[5];
    BYTE SequencerAddress;

    BYTE Crtc[25];
    BYTE CrtcAddress;

    BYTE Graphics[9];
    BYTE GraphicsAddress;

    BYTE Attribute[21];
    BYTE AttributeAddress;
    BYTE AttributeFF;

    DWORD VerticalRetraceCount;

    DWORD fColorEmulation;

    BYTE PEL_Mask;                  // Color: PEL Mask register
    int  fPelWrite;                 // Color: Read or write
    DWORD PelAddressWrite;          // What index to write
    DWORD PelAddressRead;           // What index to read
    BYTE Palette[0x400];            // Palette 256 * 3 bytes {[R][G][B]}

} TVGA;

TVGA Vga;

/******************************************************************************
*                                                                             *
*   Functions                                                                 *
*                                                                             *
******************************************************************************/

static int Vga_Initialize(void);

static DWORD Vga_fnIN(DWORD port, DWORD io_len);
static void Vga_fnOUT(DWORD port, DWORD data, DWORD io_len);

static DWORD Vga_TextMemory_fnREAD(DWORD address, DWORD data_len);
static void Vga_TextMemory_fnWRITE(DWORD address, DWORD data, DWORD data_len);
static DWORD Vga_GraphMemory_fnREAD(DWORD address, DWORD data_len);
static void Vga_GraphMemory_fnWRITE(DWORD address, DWORD data, DWORD data_len);


/******************************************************************************
*                                                                             *
*   void Vga_Register(TVDD *pVdd)                                             *
*                                                                             *
*******************************************************************************
*
*   Registration function for the virtual VGA graphics adapter
*
*   Where:
*       pVdd is the address of the virtual device desctiptor to fill in
*
******************************************************************************/
void Vga_Register(TVDD *pVdd)
{
    pVdd->Initialize = Vga_Initialize;
    pVdd->Name = "VGA";
}


/******************************************************************************
*                                                                             *
*   int Vga_Initialize()                                                      *
*                                                                             *
*******************************************************************************
*
*   Initializes virtual VGA graphics adapter
*
*   Returns:
*       0 on success
*       nonzero on fail
*
******************************************************************************/
static int Vga_Initialize()
{
    TRACE(("VGA_Initialize()"));

    memset(&Vga, 0, sizeof(TVGA));

    Vga.fColorEmulation = 1;

    Vga.MiscOutput = (1<<7) | (1<<6) | (1<<1) | (1<<0);

    // Register all standard VGA ports in both monochrome and color emulation modes

    RegisterIOPort(0x3DA, 0x3DA, Vga_fnIN, Vga_fnOUT );     // Feature Control (read, color)
    RegisterIOPort(0x3CA, 0x3CA, Vga_fnIN, NULL      );     // Feature Control (read only)
    RegisterIOPort(0x3C2, 0x3C2, Vga_fnIN, Vga_fnOUT );     // Miscellaneous Output (write)
    RegisterIOPort(0x3CC, 0x3CC, Vga_fnIN, NULL      );     // Miscellaneous Output (read only)
    RegisterIOPort(0x3C4, 0x3C5, Vga_fnIN, Vga_fnOUT );     // Sequencer
    RegisterIOPort(0x3D4, 0x3D5, Vga_fnIN, Vga_fnOUT );     // CRTC color
    RegisterIOPort(0x3CE, 0x3CF, Vga_fnIN, Vga_fnOUT );     // Graphics regs
    RegisterIOPort(0x3C0, 0x3C1, Vga_fnIN, Vga_fnOUT );     // Attribute
    RegisterIOPort(0x3C6, 0x3C9, Vga_fnIN, Vga_fnOUT );     // Color registers

    // Register a port that IBM uses for VGA-only adapter card.
    // We ignore accesses to that port

    RegisterIOPort(0x46E8, 0x46E8, DummyIOPortIN, DummyIOPortOUT );   // Ignore access

    // Register memory range that VGA mapps its text mode window

    RegisterMemoryCallback( 0xB8000, 0xBFFFF, Vga_TextMemory_fnREAD, Vga_TextMemory_fnWRITE );

    // Register memory range for VGA graphics mode

    RegisterMemoryCallback( 0xA0000, 0xAFFFF, Vga_GraphMemory_fnREAD, Vga_GraphMemory_fnWRITE );

    return( 0 );
}


//*****************************************************************************
//
//      CALLBACK FUNCTIONS
//
//*****************************************************************************

static DWORD Vga_fnIN(DWORD port, DWORD io_len)
{
    static int tick = 0;
    DWORD value;

    TRACE(("Vga_fnIN(DWORD port=%04X, DWORD io_len=%X)", port, io_len));
    ASSERT(io_len==1);

    switch( port )
    {
        //---------------------------------------------------------------------
        //  GENERAL REGISTERS
        //---------------------------------------------------------------------
        case 0x3DA:             // Input Status #1 (color)
                tick++;
                value = 0;
                if( (tick & 3)==0 )        // Fake horizontal and vertical sync
                    value = 1;
                if( (tick & 1)==1 )
                    value = 9;

                // Reading this port also resets the attribute flip-flop

                Vga.AttributeFF = 0;
            break;

        case 0x3CC:             // Miscellaneous Output / Input
                value = Vga.MiscOutput;
            break;

        case 0x3CA:
                ASSERT(0);
                value = Vga.FeatureControl; // ???
            break;

        case 0x3C2:             // Read only: Input Status #0
                // Switch Sense bit are used to determine the monitor connect
                if( (Vga.Palette[0]>0x1C) || (Vga.Palette[1]>0x1C) || (Vga.Palette[2]>0x1C) )
                    value = 0 << 4;
                else
                    value = 1 << 4;

                // Vertical retrace vs video time
                if( (tick++ & 1)==1 )
                    value |= 0x80;
            break;

        //---------------------------------------------------------------------
        //  SEQUENCER REGISTERS
        //---------------------------------------------------------------------
        case 0x3C4:             // Sequencer Address register
                value = Vga.SequencerAddress;
            break;

        case 0x3C5:             // Sequencer Data register
                value = Vga.Sequencer[ Vga.SequencerAddress ];
            break;

        //---------------------------------------------------------------------
        //  CRT CONTROLLER REGISTERS
        //---------------------------------------------------------------------
        case 0x3D4:
                value = Vga.CrtcAddress;
            break;

        case 0x3D5:
                value = Vga.Crtc[ Vga.CrtcAddress ];
            break;

        //---------------------------------------------------------------------
        //  GRAPHICS CONTROLLER REGISTERS
        //---------------------------------------------------------------------
        case 0x3CE:             // Graphics Address register
                value = Vga.GraphicsAddress;
            break;

        case 0x3CF:             // Graphics Data register
                value = Vga.Graphics[ Vga.GraphicsAddress ];
            break;

        //---------------------------------------------------------------------
        //  ATTRIBUTE CONTROLLER REGISTERS
        //---------------------------------------------------------------------
        case 0x3C1:             // Attribute read register
                value = Vga.Attribute[ Vga.AttributeAddress ];
            break;

        //---------------------------------------------------------------------
        //  COLOR REGISTERS
        //---------------------------------------------------------------------
        case 0x3C6:             // PEL Mask register
                value = Vga.PEL_Mask;
            break;

        case 0x3C7:             // Write: PEL Address Read register
                                // Read: DAC State register
                if( Vga.fPelWrite )
                    value = 3;
                else
                    value = 0;
            break;

        case 0x3C8:             // Write: PEL Address Write register
                                // Read: PEL Address Write register
                value = Vga.PelAddressWrite;
                Vga.fPelWrite = 1;
            break;

        case 0x3C9:             // PEL Data register (autoincrement)
                value = Vga.Palette[ (Vga.PelAddressRead++) & 0x3FF ];
            break;

        //---------------------------------------------------------------------
        default:
            WARNING(("Invalid register to read"));
    }

    return( value );
}


static void Vga_fnOUT(DWORD port, DWORD data, DWORD io_len)
{
    // We can use word write to VGA registers

    if( io_len==2 )
    {
        Vga_fnOUT(port, data & 0xFF, 1);

        // Proceed with the next port and high-byte data
        port++;
        data >>= 8;
        io_len = 1;
    }

    TRACE(("Vga_fnOUT(DWORD port=%04X, DWORD data=%X, DWORD io_len=%X)", port, data, io_len));
    ASSERT(io_len==1);

    switch( port )
    {
        //---------------------------------------------------------------------
        //  GENERAL REGISTERS
        //---------------------------------------------------------------------
        case 0x3C2:
                Vga.MiscOutput = (BYTE) data;
                Vga.fColorEmulation = data & 1; // Bit 0 is color emulation
                INFO(("Color emulation set to %d", Vga.fColorEmulation));
            break;

        case 0x3DA:
                Vga.FeatureControl = (BYTE) data;
            break;

        //---------------------------------------------------------------------
        //  SEQUENCER REGISTERS
        //---------------------------------------------------------------------
        case 0x3C4:             // Sequencer Address register
                if( data > 4 )
                {
                    WARNING(("Sequencer address register set too large"));
                    data = 0;
                }

                Vga.SequencerAddress = (BYTE) data;
            break;

        case 0x3C5:             // Sequencer Data register
                if( Vga.SequencerAddress==0 )
                    data |= 3;  // Reset bits should appear enabled

                Vga.Sequencer[ Vga.SequencerAddress ] = (BYTE) data;
            break;

        //---------------------------------------------------------------------
        //  CRT CONTROLLER REGISTERS
        //---------------------------------------------------------------------
        case 0x3D4:
                if( data > 0x18 )
                {
                    WARNING(("CRTC address register set too large"));
                    data = 0;
                }

                Vga.CrtcAddress = (BYTE) data;
            break;

        case 0x3D5:
                Vga.Crtc[ Vga.CrtcAddress ] = (BYTE) data;
            break;

        //---------------------------------------------------------------------
        //  GRAPHICS CONTROLLER REGISTERS
        //---------------------------------------------------------------------
        case 0x3CE:             // Graphics Address register
                if( data > 8 )
                {
                    WARNING(("Graphics address register set too large"));
                    data = 0;
                }

                Vga.GraphicsAddress = (BYTE) data;
            break;

        case 0x3CF:             // Graphics Data register
                Vga.Graphics[ Vga.GraphicsAddress ] = (BYTE) data;
            break;

        //---------------------------------------------------------------------
        //  ATTRIBUTE CONTROLLER REGISTERS
        //---------------------------------------------------------------------
        case 0x3C0:             // Attribute register
                if( Vga.AttributeFF==0 )
                {
                    if( data > 21 )         // Flip-flop selects address
                    {
                        WARNING(("Attribute address register set too large"));
                        data = 0;
                    }

                    Vga.AttributeAddress = (BYTE) data;
                }
                else                        // Flip-flop selects data
                {
                    Vga.Attribute[ Vga.AttributeAddress ] = (BYTE) data;
                }

                Vga.AttributeFF = ! Vga.AttributeFF;
            break;

        //---------------------------------------------------------------------
        //  COLOR REGISTERS
        //---------------------------------------------------------------------
        case 0x3C6:             // PEL Mask register
                Vga.PEL_Mask = (BYTE) data;
                INFO(("PEL Mask set to %02X", data));
            break;

        case 0x3C7:             // Write: PEL Address Read register
                                // Read: DAC State register
                Vga.PelAddressRead = (BYTE) data;
                Vga.fPelWrite = 0;

        case 0x3C8:             // Write: PEL Address Write register
                                // Read: PEL Address Write register
                Vga.PelAddressWrite = (BYTE) data;
                Vga.fPelWrite = 1;
            break;

        case 0x3C9:             // PEL Data register (autoincrement)
                Vga.Palette[ (Vga.PelAddressWrite++) & 0x3FF ] = (BYTE) data;
            break;

        //---------------------------------------------------------------------
        default:
            WARNING(("Invalid register to write"));
    }
}


//*****************************************************************************
//
//      MEMORY ACCESS CALLBACK FUNCTIONS
//
//*****************************************************************************

static DWORD Vga_TextMemory_fnREAD(DWORD address, DWORD data_len)
{
    ASSERT((address>=0xB8000) && (address<=0xBFFFF));

    return( *(DWORD *)&pVM->VgaTextBuffer[ address - 0xB8000 ] );
}

static void Vga_TextMemory_fnWRITE(DWORD address, DWORD data, DWORD data_len)
{
    ASSERT((address>=0xB8000) && (address<=0xBFFFF));

    switch( data_len )
    {
        case 1:     pVM->VgaTextBuffer[ address - 0xB8000 ] = (BYTE) data;             break;
        case 2:     *(WORD *)&pVM->VgaTextBuffer[ address - 0xB8000 ] = (WORD) data;   break;
        case 4:     *(DWORD *)&pVM->VgaTextBuffer[ address - 0xB8000 ] = data;         break;
        default:    ASSERT(0);
    }
}


static DWORD Vga_GraphMemory_fnREAD(DWORD address, DWORD data_len)
{
//    ASSERT(0);

    return( 0 );
}

static void Vga_GraphMemory_fnWRITE(DWORD address, DWORD data, DWORD data_len)
{
//    ASSERT(0);
}

