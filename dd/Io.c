/******************************************************************************
*                                                                             *
*   Module:     IO.c                                                          *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       4/19/2000                                                     *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the code for virtualizing Input/Output
        ports.

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 4/19/2000  Original                                             Goran Devic *
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

/******************************************************************************
*                                                                             *
*   Functions                                                                 *
*                                                                             *
******************************************************************************/

static int IoInitialize(void);

static DWORD IoDefaultIN(DWORD port, DWORD io_len);
static void IoDefaultOUT(DWORD port, DWORD data, DWORD io_len);
static DWORD P92IOPortIN(DWORD port, DWORD io_len);
static void P92IOPortOUT(DWORD port, DWORD data, DWORD io_len);

static void BochsPrintfPortOUT(DWORD port, DWORD data, DWORD io_len);


/******************************************************************************
*                                                                             *
*   void IoRegister(TVDD *pVdd)                                               *
*                                                                             *
*******************************************************************************
*
*   Registration function for the IO subsystem.
*
*   Where:
*       pVdd is the address of the virtual device desctiptor to fill in
*
******************************************************************************/
void IoRegister(TVDD *pVdd)
{
    pVdd->Initialize = IoInitialize;
    pVdd->Name = "Input/Output";
}


/******************************************************************************
*                                                                             *
*   int IoInitialize()                                                        *
*                                                                             *
*******************************************************************************
*
*   Initialization function for the IO subsystem.
*
*   Returns:
*       0 on success
*       nonzero on fail
*
******************************************************************************/
static int IoInitialize()
{
    TRACE(("IoInitialize()"));

    // Make up IO[0] to point to our stub since in the dispatcher we dont
    // check for valid indices (non-registered index is 0, not valid)

    Device.IO[0].pInCallback  = IoDefaultIN;
    Device.IO[0].pOutCallback = IoDefaultOUT;

    // Initialize dummy ports that are used, I guess, for delay and
    // (hopefully) dont do anything

    RegisterIOPort( 0xED, 0xED, DummyIOPortIN, DummyIOPortOUT );

    // Define handlers for port 0x92 that contains INIT and RESET functions

    RegisterIOPort( 0x92, 0x92, P92IOPortIN, P92IOPortOUT );

    // Register temporary Bochs printout port

    RegisterIOPort( 0xFFF0, 0xFFF0, NULL, BochsPrintfPortOUT );

    return( 0 );
}


/******************************************************************************
*                                                                             *
*   int RegisterIOPort(DWORD first_port, DWORD last_port,                     *
*                      TIO_IN_Callback fnIN, TIO_OUT_Callback fnOUT )         *
*                                                                             *
*******************************************************************************
*
*   This function registers IO ports callbacks.
*
*   Where:
*       first_port is the number of the first port to be registered: 0-65535
*       last_port is the number of the last port to be included: 0-65535
*       fnIN is the function callback when reading from a port
*       fnOUT is the function callback when writing to a port
*
*   Returns:
*       0 if the registration succeeds
*       non-zero if the port can not be registered:
*           -1 : port is already registered
*           -2 : bad parameters to a call
*           -3 : internal (out of IO structures)
*
******************************************************************************/
int RegisterIOPort(DWORD first_port, DWORD last_port,TIO_IN_Callback fnIN, TIO_OUT_Callback fnOUT )
{
    DWORD i;

    ASSERT(first_port < 65536);
    ASSERT(last_port < 65536);
    ASSERT(first_port <= last_port);

    // Make sure the IO range is not already registered

    for( i=first_port; i<=last_port; i++)
    {
        if( Device.IOIndexMap[i] != 0 )
        {
            ASSERT(0);
            return( -1 );
        }
    }

    if( (first_port<65536)&&(last_port<65536)&&(first_port<=last_port) )
    {
        // Find the first empty IO slot in our array

        for( i=1; i<MAX_IO_HND; i++ )
        {
            if( Device.IO[i].fRegistered==0 )
            {
                // Found an empty slot - register it and store pointers

                Device.IO[i].fRegistered = 1;

                Device.IO[i].FirstPort    = first_port;
                Device.IO[i].LastPort     = last_port;
                Device.IO[i].pInCallback  = fnIN;
                Device.IO[i].pOutCallback = fnOUT;

                // Set the index of our record into the port lookup array

                for( ;first_port <= last_port ; first_port++ )
                {
                    Device.IOIndexMap[first_port] = (BYTE) i;
                }
                return( 0 );
            }
        }
        return( -3 );
    }
    return( -2 );
}


//*****************************************************************************
//
//      Default function callbacks that are used to access not registered
//      IO port.
//
//*****************************************************************************

static DWORD IoDefaultIN(DWORD port, DWORD io_len)
{
    WARNING(("Accessed unregistered port 0x%04X (IN io_len %d)", port, io_len));

//    ASSERT(0);
    return( 0xFFFFFFFF );
}

static void IoDefaultOUT(DWORD port, DWORD data, DWORD io_len)
{
    WARNING(("Accessed unregistered port 0x%04X (OUT data: %02X io_len: %d", port, data, io_len));

//    ASSERT(0);
}

//*****************************************************************************
//
//      Dummy ports that dont do anything useful, but are still used with BIOS
//
//*****************************************************************************

DWORD DummyIOPortIN(DWORD port, DWORD io_len)
{
    TRACE(("DummyIOPortIN(DWORD port=%04X, DWORD io_len=%X)", port, io_len));
    return( 0xFFFFFFFF );
}

void DummyIOPortOUT(DWORD port, DWORD data, DWORD io_len)
{
    TRACE(("DummyIOPortOUT(DWORD port=%04X, DWORD data=%X, DWORD io_len=%X)", port, data, io_len));
}


//*****************************************************************************
//
//      Port 0x92 that contains FAST_A20 and FAST_INIT bits
//
//*****************************************************************************

static DWORD P92IOPortIN(DWORD port, DWORD io_len)
{
    TRACE(("P92IOPortIN(DWORD port=%04X, DWORD io_len=%X)", port, io_len));

    // Return A20 line state

    return( Mem.fA20 << 1 );
}

static void P92IOPortOUT(DWORD port, DWORD data, DWORD io_len)
{
    TRACE(("P92IOPortOUT(DWORD port=%04X, DWORD data=%X, DWORD io_len=%X)", port, data, io_len));

    if( (data & ~3) )
    {
        WARNING(("Writing reserved bits [7:2]"));
        data &= 0x0003;
    }

    // Set the new state of the A20 line and see if we need to reset the CPU

    A20SetState( (port >> 1) & 1 );

    if( (data & 1)==1 )
    {
        WARNING(("CPU FAST_INIT (reset) via port 0x92"));
        CPU.fResetPending = 1;
        ASSERT(0);
    }
}


//*****************************************************************************
//
//      Bochs ROM uses port 0xFFF0 to print out messages
//
//*****************************************************************************

static void BochsPrintfPortOUT(DWORD port, DWORD data, DWORD io_len)
{
    static char Str[256];
    static int index = 0;

    Str[index++] = (BYTE) data;
    Str[index] = 0;

    if( data=='\n' )
    {
        DebugPrint2(Str);
        index = 0;
    }
}
