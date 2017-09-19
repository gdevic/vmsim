/******************************************************************************
*                                                                             *
*   Module:     IoCtl.h                                                       *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       3/9/2000                                                      *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This is a header file defining the device driver interfaces

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 3/9/2000   Original                                             Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Important Defines                                                         *
******************************************************************************/
#ifndef _IOCTL_H_
#define _IOCTL_H_

/******************************************************************************
*                                                                             *
*   Include Files                                                             *
*                                                                             *
******************************************************************************/

#include "Types.h"                      // Include basic data types

/******************************************************************************
*                                                                             *
*   Global Defines, Variables and Macros                                      *
*                                                                             *
******************************************************************************/
// Make sure we have consistent structure packing (dword aligned)
#pragma pack( push, enter_vm )
#pragma pack( 4 )
//-----------------------------------------------------------------------------

#define BreakPoint

//-----------------------------------------------------------------------------
// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-32767, and 32768-65535 are reserved for use
// by customers.
//-----------------------------------------------------------------------------
#define FILE_DEVICE_VMSIM      0x00008305

//-----------------------------------------------------------------------------
// Commands that the GUI can send to the device driver
//-----------------------------------------------------------------------------
#define VMSIM_AllocTVM          (ULONG) CTL_CODE( FILE_DEVICE_VMSIM, 0x00, METHOD_NEITHER, FILE_ANY_ACCESS )
#define VMSIM_FreeTVM           (ULONG) CTL_CODE( FILE_DEVICE_VMSIM, 0x01, METHOD_NEITHER, FILE_ANY_ACCESS )
#define VMSIM_MapROM            (ULONG) CTL_CODE( FILE_DEVICE_VMSIM, 0x02, METHOD_IN_DIRECT, FILE_ANY_ACCESS )
#define VMSIM_MemCheck          (ULONG) CTL_CODE( FILE_DEVICE_VMSIM, 0x03, METHOD_NEITHER, FILE_ANY_ACCESS )
#define VMSIM_FindNTHole        (ULONG) CTL_CODE( FILE_DEVICE_VMSIM, 0x04, METHOD_NEITHER, FILE_ANY_ACCESS )
#define VMSIM_InitVM            (ULONG) CTL_CODE( FILE_DEVICE_VMSIM, 0x05, METHOD_NEITHER, FILE_ANY_ACCESS )
#define VMSIM_RunVM             (ULONG) CTL_CODE( FILE_DEVICE_VMSIM, 0x06, METHOD_NEITHER, FILE_ANY_ACCESS )
    // run - only completes this IRP for sync requests or after stop received
#define VMSIM_Control           (ULONG) CTL_CODE( FILE_DEVICE_VMSIM, 0x07, METHOD_NEITHER, FILE_ANY_ACCESS )
    // control - set flags/actions and complete IRP immediately
#define VMSIM_InputDevice       (ULONG) CTL_CODE( FILE_DEVICE_VMSIM, 0x08, METHOD_NEITHER, FILE_ANY_ACCESS )
    // input device - mouse / keyboard - complete IRP as soon as possible
#define VMSIM_GetVMRequest      (ULONG) CTL_CODE( FILE_DEVICE_VMSIM, 0x09, METHOD_OUT_DIRECT, FILE_ANY_ACCESS )
    // GetVMRequest - IRP that waits for async requests from VM
#define VMSIM_CompleteVMRequest (ULONG) CTL_CODE( FILE_DEVICE_VMSIM, 0x0A, METHOD_IN_DIRECT, FILE_ANY_ACCESS )
    // CompleteVMRequest - completion of VM async request
#define VMSIM_CleanItUp         (ULONG) CTL_CODE( FILE_DEVICE_VMSIM, 0x0B, METHOD_NEITHER, FILE_ANY_ACCESS )
    // CleanItUp - called when left hanging

#define VMSIM_RegisterMemory    (ULONG) CTL_CODE( FILE_DEVICE_VMSIM, 0x0C, METHOD_NEITHER, FILE_ANY_ACCESS )
#define VMSIM_RegisterIO        (ULONG) CTL_CODE( FILE_DEVICE_VMSIM, 0x0D, METHOD_NEITHER, FILE_ANY_ACCESS )
#define VMSIM_RegisterPCI       (ULONG) CTL_CODE( FILE_DEVICE_VMSIM, 0x0E, METHOD_NEITHER, FILE_ANY_ACCESS )
#define VMSIM_GenerateIRQ       (ULONG) CTL_CODE( FILE_DEVICE_VMSIM, 0x0F, METHOD_NEITHER, FILE_ANY_ACCESS )

/////////////////////////////////////////////////////////////////////////////////////////////////////
// VMControlTypes are for operational control of the VM
// These are used in VMSIM_Control
// All will complete their IRP immediately
enum VMControlTypes
{
    CtlStartVM,             // start all virtual device subsystems
    CtlStopVM,              // stop - complete all IRPs with StatusVM_Exiting, then clean up and stop
    CtlSuspendVM,           // suspend - stop processing, leave IRPS in place
    CtlResumeVM,            // resume - resume processing
    CtlResetVM,             // reset to the VM
    CtlAltDelVM             // Ctrl Alt Del combo to the VM
};

// data structure passed in with VMSIM_Control
typedef struct _VMCONTROL
{
    DWORD   type;       // type of control
    DWORD   flags;      // any associated flags
} VMCONTROL, *PVMCONTROL;


/////////////////////////////////////////////////////////////////////////////////////////////////////
// VMRequestTypes are used by the VM to request actions at Ring3
// These are used in the output of VMSIM_GetVMRequest and VMSIM_RunVM, and the input of VMSIM_CompleteVMRequest
// VMSIM_RunVM waits until the VM needs to request an sync request.
// VMSIM_GetVMRequest waits until the VM needs to request an async request.
// VMSIM_CompleteVMRequest is completed immediately

enum VMRequestTypes
{
    ReqVMExit,          // stop received, no request - no completion returned
    ReqVMSuspended,     // driver detected suspended state - sleep in the app
    ReqVMMessage,       // request a message to be displayed - no completion returned
    ReqVMReadDisk,      // read from disk - completion returned with data
    ReqVMWriteDisk,     // write to disk - completion returned when written
};

typedef void (*PCOMPLETE)(struct _VMREQUEST *, DWORD, BYTE *);

typedef struct _VMREQUEST_HEADER
{
    DWORD   type;                       // request type from a device driver
    DWORD   flags;                      // any associated flags
    DWORD   bufferlen;                  // bytes to transfer / in buffer
    PCOMPLETE pComplete;                // pointer to a driver completion routine
    DWORD   completeArg;                // argument to a completion function
    DWORD   requestID;                  // ID used in completion
    DWORD   drive;                      // logical drive
    DWORD   offset;                     // logical offset (multiple of a sector size, 512)
    DWORD   sectors;                    // number of consecutive sectors to transfer
} VMREQUEST_HEADER, *PVMREQUEST_HEADER;

// The built-in buffer should be enough to pass a message or a small structure via request
#define MAX_TRANSFER_BYTES (128 - sizeof(VMREQUEST_HEADER))

typedef struct _VMREQUEST
{
    VMREQUEST_HEADER head;              // Request header structure
    BYTE    buffer[MAX_TRANSFER_BYTES]; // Request buffer
} VMREQUEST, *PVMREQUEST;


/////////////////////////////////////////////////////////////////////////////////////////////////////
// VMInputDeviceTypes are for inputs from the user through the GUI to the hardware
// These are used in VMSIM_InputDevice
// All will complete their IRP as soon as possible
enum VMInputDeviceTypes
{
    InMouseMove,        // mouse movement
    InKeyPress,         // key press
    InSpecialKeys,      // special key sequences
    InMouseLBtnDn,      // mouse left btn down
    InMouseLBtnUp,      // mouse left btn up
    InMouseLBtnDbl,     // mouse left btn double click
    InMouseRBtnDn,      // mouse right btn down
    InMouseRBtnUp,      // mouse right btn up
    InMouseRBtnDbl,     // mouse right btn double click
};

// data structure passed in with VMSIM_InputDevice
typedef struct _VMINPUTDEVICE
{
    DWORD   type;       // type of input
    DWORD   flags;      // any associated flags
    DWORD   scan_code;  // control code - keycode or special keycode
    WORD    x;          // x location - if mouse related
    WORD    y;          // y location - if mouse related
} VMINPUTDEVICE, *PVMINPUTDEVICE;


//-----------------------------------------------------------------------------
// Restore structure packing value
#pragma pack( pop, enter_vm )

#endif //  _IOCTL_H_
