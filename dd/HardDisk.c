/******************************************************************************
*                                                                             *
*   Module:     HardDisk.c                                                    *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       6/23/2000                                                     *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the code for virtual IDE HDD device.

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 6/23/2000  Original                                             Goran Devic *
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

static int Hdd_Initialize(void);

static DWORD Hdd_fnIN(DWORD port, DWORD io_len);
static void Hdd_fnOUT(DWORD port, DWORD data, DWORD io_len);

/******************************************************************************
*                                                                             *
*   void Hdd_Register(TVDD *pVdd)                                             *
*                                                                             *
*******************************************************************************
*
*   Registration function for the virtual hard disk device.
*
*   Where:
*       pVdd is the address of the virtual device desctiptor to fill in
*
******************************************************************************/
void Hdd_Register(TVDD *pVdd)
{
    pVdd->Initialize = Hdd_Initialize;
    pVdd->Name = "HardDisk";
}


/******************************************************************************
*                                                                             *
*   int Hdd_Initialize()                                                      *
*                                                                             *
*******************************************************************************
*
*   Initializes virtual hard disk device interface
*
*   Returns:
*       0 on success
*       nonzero on fail
*
******************************************************************************/
static int Hdd_Initialize()
{
    TRACE(("Hdd_Initialize()"));

//    memset(&c, 0, sizeof(c));

    // Register a hard disk controller ports

    RegisterIOPort(0x1F0, 0x1F7, Hdd_fnIN, Hdd_fnOUT );

    return( 0 );
}


//*****************************************************************************
//
//      CALLBACK FUNCTIONS
//
//*****************************************************************************

static DWORD Hdd_fnIN(DWORD port, DWORD io_len)
{
    DWORD value=0;
    TRACE(("Hdd_fnIN(DWORD port=%04X, DWORD io_len=%X)", port, io_len));

    return( value );
}


static void Hdd_fnOUT(DWORD port, DWORD data, DWORD io_len)
{
    TRACE(("Hdd_fnOUT(DWORD port=%04X, DWORD data=%X, DWORD io_len=%X)", port, data, io_len));

}
