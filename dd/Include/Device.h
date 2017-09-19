/******************************************************************************
*                                                                             *
*   Module:     Device.h                                                      *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       5/22/2000                                                     *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This is a header file containing defines and structures used by
        virtual devices.

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 5/22/2000  Original                                             Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Important Defines                                                         *
******************************************************************************/
#ifndef _DEVICE_H_
#define _DEVICE_H_

/******************************************************************************
*                                                                             *
*   Global Defines, Variables and Macros                                      *
*                                                                             *
******************************************************************************/

//-----------------------------------------------------------------------------
// Define input/output handler data structure
//-----------------------------------------------------------------------------
typedef struct
{
    DWORD fRegistered;                      // This slot is already registered

    DWORD FirstPort;                        // First IO registered port
    DWORD LastPort;                         // Last IO registered port

    TIO_IN_Callback  pInCallback;           // Callback for 'IN' instruction
    TIO_OUT_Callback pOutCallback;          // Callback for 'OUT' instruction

} TIO;

//-----------------------------------------------------------------------------
// Define PCI configuration space handler data structure
//-----------------------------------------------------------------------------

typedef struct
{
    DWORD fRegistered;                      // This slot is already registered
    DWORD reserved;                         // not used (only to align the size)

    TPCI_IN_Callback  pPciInCallback;       // Callback for PCI 'IN' instruction
    TPCI_OUT_Callback pPciOutCallback;      // Callback for PCI 'OUT' instruction

} TPCI_IO;

//-----------------------------------------------------------------------------
// Define memory handler data structure
//-----------------------------------------------------------------------------
typedef struct
{
    DWORD BaseAddress;                      // Base physical region address
    DWORD LastAddress;                      // Last byte of the region

    TMEM_READ_Callback  pReadCallback;      // Callback for read data
    TMEM_WRITE_Callback pWriteCallback;     // Callback for write data

} TMEM;


//-----------------------------------------------------------------------------
// Descriptor structure defining a virtual device driver callback functions
//  A virtual device is called first through the XX_Register() function.
//  It fills in arbitrary number of handlers (Initialize and Name are mandatory).
//  Initialize() should do necessary initialization, fault registration and
//  memory allocation.
//  Start() is called immediately prior to actual VM spin.  Device should start
//  timers or othervise prepare for run.
//  Suspend()/Resume() are called on pausing and resuming of a VM execution.
//  Shutdown() should disable and clean up a device memory.
//-----------------------------------------------------------------------------

typedef struct
{
    int (* Initialize)(void);               // Initialization function
    int (* Start)(void);                    // Start function
    int (* Suspend)(void);                  // Suspend function
    int (* Resume)(void);                   // Resume function
    int (* Shutdown)(void);                 // Shutdown function
    char *Name;                             // Pointer to a device name

} TVDD;

//-----------------------------------------------------------------------------
// Main device structure that contains data pertinent to each major device
//-----------------------------------------------------------------------------

typedef struct
{
    TVDD    Vdd[ MAX_VDD ];                 // List of virtual device descriptors
    DWORD   VddTop;                         // Index of the last registered one

    // -------------------------------------------------------------------------------
    // IO subsystem index map for 64K of IO ports.  Values index to IO[] for a
    // registered handler.  Index 0 is not valid and means the port is not registered.
    // -------------------------------------------------------------------------------
    BYTE IOIndexMap[65536];     // Index array for IO port handlers, indexes:
    TIO IO[ MAX_IO_HND ];       // Array of input/output handlers

    // -------------------------------------------------------------------------------
    // PCI configuration space: array of structures of PCI handlers for each device.
    // -------------------------------------------------------------------------------
    TPCI_IO PCI[ MAX_PCI_DEVICES ];

    // -------------------------------------------------------------------------------
    // Memory access callback structures
    // -------------------------------------------------------------------------------
    TMEM MEM[ MAX_MEM_HND ];                // Memory ranges records
    DWORD MemTop;                           // Index of the first available record

} TDevice;


#endif //  _DEVICE_H_
