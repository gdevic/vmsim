/******************************************************************************
*                                                                             *
*   Module:     Interface.h                                                   *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       4/19/2000                                                     *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This header file contains the function prototypes and structures
        that are used to interface to the virtual machine simulator.

        These comprise the main interface for writing a virtual device.

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 4/19/2000  Original                                             Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Important Defines                                                         *
******************************************************************************/
#ifndef _INTERFACE_H_
#define _INTERFACE_H_

/******************************************************************************
*                                                                             *
*   Include Files                                                             *
*                                                                             *
******************************************************************************/

/******************************************************************************
*                                                                             *
*   Global Defines, Variables and Macros                                      *
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

//============================================================================
// Define memory callback functions
//============================================================================

typedef DWORD (*TMEM_READ_Callback)(DWORD address, DWORD data_len);
typedef void (*TMEM_WRITE_Callback)(DWORD address, DWORD data, DWORD data_len);

//============================================================================
// Registration function: Call this function to register your own physical
// memory arenas to trap access to.
//============================================================================

extern int RegisterMemoryCallback( DWORD physical_address, DWORD last_address,
                      TMEM_READ_Callback fnREAD, TMEM_WRITE_Callback fnWRITE );


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
// Define I/O callback functions
//============================================================================

typedef DWORD (*TIO_IN_Callback)(DWORD port, DWORD io_len);
typedef void (*TIO_OUT_Callback)(DWORD port, DWORD data, DWORD io_len);

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

extern int RegisterIOPort(DWORD first_port, DWORD last_port,
                          TIO_IN_Callback fnIN, TIO_OUT_Callback fnOUT );


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
// Define standard type zero PCI header
// This typedef is provided to ease prototyping your own PCI devices
//============================================================================
typedef struct
{
    WORD    vendorID;
    WORD    deviceID;
    WORD    command;
    WORD    status;
    DWORD   revisionClass;
    BYTE    cacheLineSize;
    BYTE    latency;
    BYTE    headerType;
    BYTE    bist;
    DWORD   baseAddress0;
    DWORD   baseAddress1;
    DWORD   baseAddress2;
    DWORD   baseAddress3;
    DWORD   baseAddress4;
    DWORD   baseAddress5;
    DWORD   cardBus;
    WORD    subsistemVendorID;
    WORD    subsystemID;
    DWORD   romBaseAddress;
    DWORD   reserved1;
    DWORD   reserved2;
    BYTE    interruptLine;
    BYTE    interruptPin;
    BYTE    minGnt;
    BYTE    maxLatency;
    DWORD   deviceSpecific[48];

} TPciHeader;

//============================================================================
// Define PCI callback functions: These callback functions will be called
// when a virtual machine accesses PCI configuration space that you registered
//============================================================================

typedef DWORD (*TPCI_IN_Callback)(DWORD function, DWORD index);
typedef void (*TPCI_OUT_Callback)(DWORD function, DWORD index, DWORD data);

//============================================================================
// Registration function: Call this function to register your own
// handler for a PCI configuration space access.
// Device number can be 0-31 (7 is reserved for internal PIIX4 device)
//
// Returns:
//      0 if the registration succeeds
//      non-zero if the registration fails (no change)
//============================================================================

extern int RegisterPCIDevice(DWORD device_number,
                             TPCI_IN_Callback fnIN, TPCI_OUT_Callback fnOUT );


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
// Interrupt number can be 0-15 (excluding 2 that is cascaded)
//============================================================================

extern int VmSIMGenerateInterrupt(DWORD IRQ_number);


#endif //  _INTERFACE_H_
