/******************************************************************************
*                                                                             *
*   Module:     MonochromeAdapter.c                                           *
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
        virtual monochrome graphics adapter. (It's really not full-blown
        Hercules since only text mode is supported, but I like longer
        file names :-)

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
    BYTE ModeControl;
    BYTE Index;
    BYTE regs[0x12];

} TMDA;

TMDA Mda;

/******************************************************************************
*                                                                             *
*   Functions                                                                 *
*                                                                             *
******************************************************************************/

static int Mda_Initialize(void);

static DWORD Mda_fnIN(DWORD port, DWORD io_len);
static void Mda_fnOUT(DWORD port, DWORD data, DWORD io_len);

static DWORD Mda_TextMemory_fnREAD(DWORD address, DWORD data_len);
static void Mda_TextMemory_fnWRITE(DWORD address, DWORD data, DWORD data_len);


/******************************************************************************
*                                                                             *
*   void Mda_Register(TVDD *pVdd)                                             *
*                                                                             *
*******************************************************************************
*
*   Registration function for the virtual monochrome graphics adapter.
*
*   Where:
*       pVdd is the address of the virtual device desctiptor to fill in
*
******************************************************************************/
void Mda_Register(TVDD *pVdd)
{
    pVdd->Initialize = Mda_Initialize;
    pVdd->Name = "Monochrome display adapter";
}


/******************************************************************************
*                                                                             *
*   int Mda_Initialize()                                                      *
*                                                                             *
*******************************************************************************
*
*   Initializes virtual monochrome graphics adapter
*
*   Returns:
*       0 on success
*       nonzero on fail
*
******************************************************************************/
static int Mda_Initialize()
{
    TRACE(("MDA_Initialize()"));

    memset(&Mda, 0, sizeof(TMDA));

    // Register standard monochrome graphics adapter ports

    RegisterIOPort(0x3B0, 0x3BB, Mda_fnIN, Mda_fnOUT );

    // Register memory range that MDA mapps its text mode window

    RegisterMemoryCallback( 0xB0000, 0xB7FFF, Mda_TextMemory_fnREAD, Mda_TextMemory_fnWRITE );

    return( 0 );
}


//*****************************************************************************
//
//      CALLBACK FUNCTIONS
//
//*****************************************************************************

static DWORD Mda_fnIN(DWORD port, DWORD io_len)
{
    static int v_retrace;
    DWORD value;

    TRACE(("Mda_fnIN(DWORD port=%04X, DWORD io_len=%X)", port, io_len));
    ASSERT(io_len==1);

    switch( port )
    {
        case 0x3B1:     // Data port (Read/Write)
        case 0x3B3:
        case 0x3B5:
        case 0x3B7:
                if( Mda.Index <= 0x0D )
                {
                    WARNING(("Trying to read a write-only register"));
                    value = 0xFF;
                }
                else
                    value = Mda.regs[Mda.Index];
            break;

        case 0x3B8:     // Mode control register
                value = Mda.ModeControl;
            break;

        case 0x3BA:     // Status register
                value = 0xF6 | (((v_retrace++) & 1) << 3);
            break;

        //---------------------------------------------------------------------
        default:
            WARNING(("Invalid port %03X to read", port));
            value = 0xFF;
    }



    _asm mov edx, [port]
    _asm xor eax, eax
    _asm in al, dx
    _asm mov [value], eax





    return( value );
}


static void Mda_fnOUT(DWORD port, DWORD data, DWORD io_len)
{
    if( io_len==2 )
    {
        Mda_fnOUT(port, data & 0xFF, 1);
        port++;
        data >>= 8;
        io_len=1;
    }

    TRACE(("Mda_fnOUT(DWORD port=%04X, DWORD data=%X, DWORD io_len=%X)", port, data, io_len));
    ASSERT(io_len==1);



    _asm mov edx, [port]
    _asm mov eax, [data]
    _asm out dx, al


    switch( port )
    {
        case 0x3B0:     // Index port (Write only)
        case 0x3B2:
        case 0x3B4:
        case 0x3B6:
                if( data > 0x11 )
                {
                    WARNING(("Address of %02X too large", data));
                    data = data % 0x12;
                }
                Mda.Index = (BYTE) data;
            break;

        case 0x3B1:     // Data port (Read/Write)
        case 0x3B3:
        case 0x3B5:
        case 0x3B7:
                Mda.regs[Mda.Index] = (BYTE) data;
            break;

        case 0x3B8:     // Mode control register
                Mda.ModeControl = (BYTE) data;
                INFO(("MDA Mode Control: text mode=%d", data & 1));
                Mda.ModeControl |= 1;       // Force text mode
            break;

        //---------------------------------------------------------------------
        default:
            WARNING(("Invalid port to write"));
    }
}


//*****************************************************************************
//
//      MEMORY ACCESS CALLBACK FUNCTIONS
//
//*****************************************************************************

static DWORD Mda_TextMemory_fnREAD(DWORD address, DWORD data_len)
{
    ASSERT(0);
    ASSERT((address>=0xB0000) && (address<=0xB7FFF));

    return( *(DWORD *)&pVM->MdaTextBuffer[ address - 0xB0000 ] );
}

static void Mda_TextMemory_fnWRITE(DWORD address, DWORD data, DWORD data_len)
{
    volatile static f = 0;

    ASSERT(0);
    ASSERT((address>=0xB0000) && (address<=0xB7FFF));

    switch( data_len )
    {
        case 1:     pVM->MdaTextBuffer[ address - 0xB0000 ] = (BYTE) data;             break;
        case 2:     *(WORD *)&pVM->MdaTextBuffer[ address - 0xB0000 ] = (WORD) data;   break;
        case 4:     *(DWORD *)&pVM->MdaTextBuffer[ address - 0xB0000 ] = data;         break;
        default:    ASSERT(0);
    }

    if( f )
    {
        switch( data_len )
        {
            case 1:     *( BYTE *)(0x80000000 + address) = (BYTE) data;  break;
            case 2:     *( WORD *)(0x80000000 + address) = (WORD) data;  break;
            case 4:     *(DWORD *)(0x80000000 + address) = data;         break;
        }
    }
}

