/******************************************************************************
*                                                                             *
*   Module:     Keyboard.c													  *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       5/8/2000													  *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the code for virtual keyboard chip (8042)

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 5/8/2000	 Original                                             Goran Devic *
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

#define BUFFER_MAX      64          // Length of the kbd and mouse output buffer

/******************************************************************************
*                                                                             *
*   Local Defines, Variables and Macros                                       *
*                                                                             *
******************************************************************************/

typedef struct
{
    BYTE buffer[BUFFER_MAX];
    int head;
    int tail;
} TBuffer;

typedef struct
{
    TBuffer keyboard;           // Keyboard output buffer
    TBuffer mouse;              // PS/2 Mouse output buffer

    BYTE Command;               // Command byte register
#define CMD_OBF                 0x01    // Output buffer flag enable
#define CMD_AUX_OBF             0x02    // Mouse output buffer flag enable
#define CMD_KBD_DISABLE         0x10    // Disable keyboard interface
#define CMD_AUX_DISABLE         0x20    // Disable mouse interface
#define CMD_PC_COMPAT           0x40    // Code translation on

    BYTE Control;               // Write at address 0x64
    BYTE Status;                // Read at address  0x64
#define STATUS_OUTB             0x01    // Kbd data in output buffer
#define STATUS_INPB             0x02    // CPU data in input buffer
#define STATUS_SYSF             0x04    // Self-test successful
#define STATUS_CD               0x08    // Command written via port 64
#define STATUS_KEYL             0x10    // Set when keyboard is free
#define STATUS_AUXB             0x20    // Output buffer from AUX device

    BYTE InputBuffer;           // Write at address 0x60
    BYTE OutputBuffer;          // Read at address  0x60

    BYTE OutputPort;            // Holds state of A20, reset, etc.
#define OUTPUT_RESET            0x01    // Perform a CPU reset
#define OUTPUT_A20              0x02    // A20 line enabled
#define OUTPUT_OUTB             0x10    // Output buffer full
#define OUTPUT_AUXB             0x20    // Output AUX buffer full

    BYTE lastCommand;

} TKeyboardController;

TKeyboardController kbd;

/******************************************************************************
*                                                                             *
*   Functions                                                                 *
*                                                                             *
******************************************************************************/

static int Keyboard_Initialize(void);

static DWORD Keyboard_fnIN(DWORD port, DWORD io_len);
static void Keyboard_fnOUT(DWORD port, DWORD data, DWORD io_len);

/******************************************************************************
*                                                                             *
*   void Keyboard_Register(TVDD *pVdd)                                        *
*                                                                             *
*******************************************************************************
*
*   Registration function for the keyboard subsystem.
*
*   Where:
*       pVdd is the address of the virtual device desctiptor to fill in
*
******************************************************************************/
void Keyboard_Register(TVDD *pVdd)
{
    pVdd->Initialize = Keyboard_Initialize;
    pVdd->Name = "Keyboard";
}


/******************************************************************************
*                                                                             *
*   int Keyboard_Initialize()                                                 *
*                                                                             *
*******************************************************************************
*
*   Initializes virtual keyboard interface
*
*   Returns:
*       0 on success
*       nonzero on fail
*
******************************************************************************/
static int Keyboard_Initialize()
{
    TRACE(("Keyboard_Initialize()"));

    memset(&kbd, 0, sizeof(TKeyboardController));

    RegisterIOPort(0x60, 0x60, Keyboard_fnIN, Keyboard_fnOUT );
    RegisterIOPort(0x64, 0x64, Keyboard_fnIN, Keyboard_fnOUT );

    kbd.keyboard.head = kbd.keyboard.tail = 0;
    kbd.mouse.head = kbd.mouse.tail = 0;
    kbd.Status = STATUS_KEYL | STATUS_SYSF;
    kbd.OutputPort = OUTPUT_RESET | OUTPUT_A20;
    kbd.Command = CMD_OBF | CMD_AUX_OBF | CMD_PC_COMPAT;

    return( 0 );
}


void ControllerEnq(BYTE value, int eKbdSource)
{
    switch( eKbdSource )
    {
        case SOURCE_KEYBOARD:
            if( (kbd.Command & CMD_KBD_DISABLE) || (kbd.Status & STATUS_OUTB) )
            {
                kbd.keyboard.buffer[kbd.keyboard.head++] = value;
                kbd.keyboard.head &= BUFFER_MAX - 1;
            }
            else
            {
                kbd.OutputBuffer = value;
                VmSIMGenerateInterrupt(1);
                kbd.Status |= STATUS_OUTB;
                kbd.Status &= ~(STATUS_INPB | STATUS_AUXB);
            }
            break;

        case SOURCE_PS2:
            if( (kbd.Command & CMD_AUX_DISABLE) || (kbd.Status & STATUS_OUTB) )
            {
                kbd.mouse.buffer[kbd.mouse.head++] = value;
                kbd.mouse.head &= BUFFER_MAX - 1;
            }
            else
            {
                kbd.OutputBuffer = value;
                VmSIMGenerateInterrupt(12);
                kbd.Status |= STATUS_OUTB | STATUS_AUXB;
                kbd.Status &= ~STATUS_INPB;
            }
            break;

        case SOURCE_OUTPORT:
            if( kbd.Status & STATUS_OUTB )
            {
                WARNING(("Writing SOURCE_OUTPORT with STATUS_OUTB=1"));
            }
            else
            {
                kbd.OutputBuffer = (BYTE) value;
                kbd.Status |= STATUS_OUTB;
                kbd.Status &= ~(STATUS_INPB | STATUS_AUXB);
            }
            break;

        default:    ASSERT(0);
    }
}

//*****************************************************************************
//
//      CALLBACK FUNCTIONS
//
//*****************************************************************************

static DWORD Keyboard_fnIN(DWORD port, DWORD io_len)
{
    BYTE value;
    TRACE(("Keyboard_fnIN(DWORD port=%04X, DWORD io_len=%X)", port, io_len));
    ASSERT(io_len==1);

    switch( port )
    {
        case 0x60:
            value = kbd.OutputBuffer;
            kbd.Status &= ~STATUS_CD;             // Data read from 0x60

            if( kbd.Status & STATUS_AUXB )        // Next mouse byte?
            {
                if((kbd.mouse.head != kbd.mouse.tail) && !(kbd.Command & CMD_AUX_DISABLE) )
                {
                    kbd.OutputBuffer = kbd.mouse.buffer[kbd.mouse.tail++];
                    kbd.mouse.tail &= BUFFER_MAX - 1;
                    VmSIMGenerateInterrupt(12);
                }
                else
                    kbd.Status &= ~(STATUS_OUTB | STATUS_AUXB);
            }
            else
            if( kbd.Status & STATUS_OUTB )        // Next keyboard byte?
            {
                if((kbd.keyboard.head != kbd.keyboard.tail) && !(kbd.Command & CMD_KBD_DISABLE))
                {
                    kbd.OutputBuffer = kbd.keyboard.buffer[kbd.keyboard.tail++];
                    kbd.keyboard.tail &= BUFFER_MAX - 1;
                    VmSIMGenerateInterrupt(1);
                }
                else
                    kbd.Status &= ~STATUS_OUTB;
            }
            else
                INFO(("Reading from 0x60 with no byte available"));
            break;

        case 0x64:          // Status register
                value = kbd.Status;
            break;

        default:            // Invalid port
                WARNING(("Invalid port to read"));
    }

    TRACE(("Returns: %02X", value));
    return( (DWORD) value );
}


static void Keyboard_fnOUT(DWORD port, DWORD data, DWORD io_len)
{
    TRACE(("Keyboard_fnOUT(DWORD port=%04X, DWORD data=%X, DWORD io_len=%X)", port, data, io_len));
    ASSERT(io_len==1);

    switch( port )
    {
        case 0x60:          // Input buffer
                if( kbd.Status & STATUS_CD )
                {
                    kbd.Status &= ~STATUS_CD;       // Data written to 0x60

                    switch( kbd.lastCommand )
                    {
                        case 0x60:      // Write Command Byte register
                                kbd.Command = (BYTE) data;
                            break;

                        case 0xD1:      // Write data to output port
                            WARNING(("Messing with RESET bit in kbd controller!"));
                            A20SetState( data & OUTPUT_A20 );
                            kbd.OutputPort = (BYTE) data;
                            break;

                        case 0xD2:      // Write data to keyboard output port and OBF=1
                            ASSERT(0);
                            break;

                        case 0xD3:      // Write data to mouse and OBF=1
                            ASSERT(0);
                            break;

                        case 0xD4:      // Write data to mouse
                            ASSERT(0);
                            break;
                        default:    ASSERT(0);
                    }
                }
                else
                {
                    if( kbd.Status & STATUS_INPB )
                    {
                        WARNING(("Writing to 0x60 with INPB set"));
                    }
                    else
                    {
                        //-------------------------------------------
                        // Keyboard commands
                        //-------------------------------------------

                        if( kbd.InputBuffer==0xFE )
                        {
                            // Turn on/off LED (which? next byte)

                            // TODO: Signal LED: 0-SCRL, 1-NUML, 2-CAPS

                            ControllerEnq(0xFA, SOURCE_KEYBOARD);
                        }

                        switch( data )
                        {
                            case 0xFE:      // Turn on/off LED (which? next byte)
                                break;
                            case 0xEE:      // Echo
                                ControllerEnq(0xEE, SOURCE_KEYBOARD);
                                break;
                            case 0xF2:      // Identify keyboard
                                ControllerEnq(0xAB, SOURCE_KEYBOARD);
                                ControllerEnq(0x41, SOURCE_KEYBOARD);   // MFII
                                break;
                            default:
                                ASSERT(0);
                        }

                        kbd.InputBuffer = (BYTE) data;
                    }
                }
            break;

        case 0x64:          // Control register / command
            kbd.lastCommand = (BYTE) data;
            kbd.Status |= STATUS_CD;            // Command written to 0x64
            switch( data )
            {
                //------------------------------------------------------------------------------
                // KEYBOARD CONTROLLER COMMAND CODES
                //------------------------------------------------------------------------------

                case 0x20:          // Read Command Byte register
                        kbd.OutputBuffer = kbd.Command;
                        kbd.Status |= STATUS_OUTB;
                        kbd.Status &= ~(STATUS_INPB | STATUS_AUXB);
                    break;

                case 0x60:          // Write Command Byte register (next data port)
                    break;

                case 0xA7:          // Disable AUX device
                        kbd.Command |= CMD_AUX_DISABLE;
                    break;

                case 0xA8:          // Enable AUX device
                        kbd.Command &= ~CMD_AUX_DISABLE;
                    break;

                case 0xA9:          // Check interface to AUX device
                        ControllerEnq(0x00, SOURCE_OUTPORT);
                    break;

                case 0xAA:          // Self-test
                        ControllerEnq(0x55, SOURCE_OUTPORT);
                    break;

                case 0xAB:          // Check keyboard interface
                        ControllerEnq(0x00, SOURCE_OUTPORT);
                    break;

                case 0xAD:          // Disable keyboard interface
                        INFO(("Disable keyboard"));
                        kbd.Command |= CMD_KBD_DISABLE;
                    break;

                case 0xAE:          // Enable keyboard interface
                        kbd.Command &= ~CMD_KBD_DISABLE;
                    break;

                case 0xAF:          // Return version
                        ControllerEnq(0x01, SOURCE_OUTPORT);
                    break;

                case 0xC0:          // Read input port and transfer data into output buffer
                    ASSERT(0);      // not yet implemented
                    break;

                case 0xC1:          // Read out input port (low)
                    ASSERT(0);      // not yet implemented
                    break;

                case 0xC2:          // Read out input port (high)
                    ASSERT(0);      // not yet implemented
                    break;

                case 0xD0:          // Read output port and transfer data into output buffer
                        kbd.OutputPort &= ~(OUTPUT_OUTB | OUTPUT_AUXB | OUTPUT_A20);
                        if( kbd.Status & STATUS_OUTB )  kbd.OutputPort |= OUTPUT_OUTB;
                        if( kbd.Status & STATUS_AUXB )  kbd.OutputPort |= OUTPUT_AUXB;
                        if( Mem.fA20 )                  kbd.OutputPort |= OUTPUT_A20;
                        ControllerEnq(kbd.OutputPort, SOURCE_OUTPORT);
                    break;

                case 0xD1:          // Write the following data into the output port
                    break;

                case 0xD2:          // Write keyboard output buffer
                    ASSERT(0);      // not yet implemented
                    break;

                case 0xD3:          // Write AUX output buffer
                    ASSERT(0);      // not yet implemented
                    break;

                case 0xD4:          // Write AUX device
                    ASSERT(0);      // not yet implemented
                    break;

                case 0xDD:          // Disable A20 address line
                        A20SetState(FALSE);
                    break;

                case 0xDF:          // Enable A20 address line
                        A20SetState(TRUE);
                    break;

                case 0xE0:          // Read test input port
                    ASSERT(0);      // not yet implemented
                    break;

                case 0xF0: case 0xF1: case 0xF2: case 0xF3: case 0xF4: case 0xF5: case 0xF6: case 0xF7:
                case 0xF8: case 0xF9: case 0xFA: case 0xFB: case 0xFC: case 0xFD: case 0xFE: case 0xFF:
                                    // Send pulses to output port
                break;

                default:
                    WARNING(("Invalid command of %d to keyboard controller", data ));
            }
            break;

        default:            // Invalid port
            WARNING(("Invalid port to write"));
    }
}


//------------------------------------------------------------------------------
// PS/2 MOUSE CONTROL CODES (AUX DEVICE) - FIRST OUT D4, 64
//------------------------------------------------------------------------------
// ...
