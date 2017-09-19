/******************************************************************************
*                                                                             *
*   Module:     Interface.cpp                                                 *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       5/30/2000                                                     *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the code for the Ring-3 Application level
        interface.

        Interfaces defined here are mostly stubs to a simulation core
        that resides in a device driver.

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 5/30/2000  Original                                             Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Include Files                                                             *
******************************************************************************/
#include "stdafx.h"
#include "VMgui.h"

#include <winioctl.h>
#include "Ioctl.h"
#include "vm.h"

#include "Interface.h"
#include "Simulator.h"

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

/******************************************************************************
*                                                                             *
*   MEMORY ACCESS SUBSYSTEM                                                   *
*                                                                             *
* When a code running in the VM accesses memory, if the memory range is       *
* registered, the simulator will call the registered callback function with   *
* the access address, data and length.                                        *
*                                                                             *
* The memory arenas that can be registered are defined in physical address    *
* space and may be set at any time, ex. after the PCI configuration space     *
* had been set.                                                               *
*                                                                             *
******************************************************************************/
int CSimulator::RegisterMemoryCallback( DWORD physical_address, DWORD last_address,
                      TMEM_READ_Callback fnREAD, TMEM_WRITE_Callback fnWRITE )
{
    OVERLAPPED overlap = { 0 };
    DWORD status, nb;
    DWORD params[4];
    int ret;

    params[0] = physical_address;
    params[1] = last_address;
    params[2] = (DWORD) fnREAD;
    params[3] = (DWORD) fnWRITE;

    status = DeviceIoControl( sys_handle, VMSIM_RegisterMemory,
        &params, sizeof(params),
        &ret, sizeof(ret),
        &nb, &overlap );
    return( ret );
}


/******************************************************************************
*                                                                             *
*   INPUT/OUTPUT SUBSYSTEM                                                    *
*                                                                             *
* When a virtual machine executes IN instruction that accesses a port that    *
* you registered, fnIN will be called returning the value to be passed back.  *
* When an OUT instruction is executed, fnOUT will be called.                  *
* For repeating IO, handlers will be called multiple times.                   *
*                                                                             *
******************************************************************************/
//============================================================================
// Registration function: Call this function to register your own port handlers
// Simulator will call your callback functions when an IO is performed on a
// specified IO port range.
// first_port and last_port are ports in the range [0,65535] and
// first_port <= last_port.
//
// Returns:
//      0 if the registration succeeds
//      non-zero if the registration fails (no change)
//============================================================================
int CSimulator::RegisterIOPort(DWORD first_port, DWORD last_port,
                          TIO_IN_Callback fnIN, TIO_OUT_Callback fnOUT )
{
    OVERLAPPED overlap = { 0 };
    DWORD status, nb;
    DWORD params[4];
    int ret;

    params[0] = first_port;
    params[1] = last_port;
    params[2] = (DWORD) fnIN;
    params[3] = (DWORD) fnOUT;

    status = DeviceIoControl( sys_handle, VMSIM_RegisterIO,
        &params, sizeof(params),
        &ret, sizeof(ret),
        &nb, &overlap );
    return( ret );
}


/******************************************************************************
*                                                                             *
*   PCI SUBSYSTEM                                                             *
*                                                                             *
* The access method used is "Configuration mechanism #1", as specified in     *
* the revision 2.0 and 2.1 of the PCI specificatons.  Ports used are CF8/CFB. *
*                                                                             *
* However, you may not register a custom IO handler for those ports as their  *
* handlers are part of simulator code.  Bus 0 is implied and supported.       *
*                                                                             *
* You have to allocate and keep configuration space data - simulator does not *
* keep your config space, but routes all accesses to your callback functions. *
*                                                                             *
******************************************************************************/
//============================================================================
// Registration function: Call this function to register your own
// handler for a PCI configuration space access.
// Device number can be 0-31 (7 is reserved for internal PIIX4 device)
//
// Returns:
//      0 if the registration succeeds
//      non-zero if the registration fails (no change)
//============================================================================
int CSimulator::RegisterPCIDevice(DWORD device_number, TPCI_IN_Callback fnIN, TPCI_OUT_Callback fnOUT )
{
    OVERLAPPED overlap = { 0 };
    DWORD status, nb;
    DWORD params[3];
    int ret;

    params[0] = device_number;
    params[1] = (DWORD) fnIN;
    params[2] = (DWORD) fnOUT;

    status = DeviceIoControl( sys_handle, VMSIM_RegisterPCI,
        &params, sizeof(params),
        &ret, sizeof(ret),
        &nb, &overlap );
    return( ret );
}


/******************************************************************************
*                                                                             *
*   INTERRUPT GENERATION                                                      *
*                                                                             *
* By calling the VmSIMGenerateInterrupt(), you can inject an interrupt request*
* into the PIC of the virtual machine. Depending on the state of the virtual  *
* interrupt flags, it may or may not be serviced right away.                  *
*                                                                             *
******************************************************************************/
//============================================================================
// Returns:
//  0 if interrupt posted successfully
//  1 if the interrupt number was not correct
//============================================================================
int CSimulator::VmSIMGenerateInterrupt(DWORD IRQ_number)
{
    OVERLAPPED overlap = { 0 };
    DWORD status, nb;
    DWORD params[1];
    int ret;

    params[0] = IRQ_number;

    status = DeviceIoControl( sys_handle, VMSIM_RegisterPCI,
        &params, sizeof(params),
        &ret, sizeof(ret),
        &nb, &overlap );
    return( ret );
}

