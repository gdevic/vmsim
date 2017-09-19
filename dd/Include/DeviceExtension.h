/******************************************************************************
*                                                                             *
*   Module:     DeviceExtension.h                                             *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       5/16/2000                                                     *
*                                                                             *
*   Author:     Garry Amann                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the deviceExtension for the DriverObject

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 5/16/2000  Original                                             Garry Amann *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Include Files                                                             *
******************************************************************************/
#ifndef _DEVICEEXTENSION_H_
#define _DEVICEEXTENSION_H_

#include "Queue.h"                  // Include some queue functions

#include "IoCtl.h"                  // Include ioctl defines

/******************************************************************************
*                                                                             *
*   Defines & enums                                                           *
*                                                                             *
******************************************************************************/

enum VMRuningState
{
    vmStateStopped = 0,
    vmStateRunning,
    vmStateStopRequested,
    vmStateSuspended,
};

/******************************************************************************
*                                                                             *
*   DeviceExtension                                                           *
*                                                                             *
******************************************************************************/

typedef struct _DEVICE_EXTENSION
{
    DWORD    runState;          // What state are we in generally

    //-------------------------------------------------------------------------
    // Request irp kept in the driver
    //-------------------------------------------------------------------------
    PIRP        VMRequestIrp;   // Asynchronous request management
    PVMREQUEST  pVmRequest;     // System address of the request structure
    KSPIN_LOCK  SpinLockReq;    // Spin lock on request queue
    DWORD       requestID;      // Current request number
    TQueue     *qVmReq;         // Request queue

    PIRP     VMRunIrp;
    VMREQUEST *pSyncVmReq;      // Synchronous run request buffer (vmRun)

    //-------------------------------------------------------------------------
    // CMOS and RTC use some light timers, so these are their control variables
    //-------------------------------------------------------------------------
    KDPC     VmRtcDpcA;         // Pointer to DPC of the CMOS/RTC device timer A
    KTIMER   VmRtcTimerA;       // 1-second RTC timer (A)
    LARGE_INTEGER PeriodicTimeA;// Timer value for the periodic timer (A)
    KDPC     VmRtcDpcB;         // Pointer to DPC of the CMOS/RTC device timer B
    KTIMER   VmRtcTimerB;       // Programmable periodic RTC timer (B)
    LARGE_INTEGER PeriodicTimeB;// Timer value for the periodic timer (B)
//    BOOL     fIgnorePI;         // Ignore occurrences of a periodic timer
//    BOOL     fEnableUI;         // Enable occurences of the update timer (A)

    //-------------------------------------------------------------------------
    // Periodic Interrupt Timer 0 for generating IRQ 0
    //-------------------------------------------------------------------------
    KDPC     VmPitDpc0;         // Pointer to DPC of the PIT counter 0
    KTIMER   VmPitTimer0;       // PIT timer 0

    //-------------------------------------------------------------------------

    DWORD    dwMyCtr;
    PVOID    hCodeSection;      // Handle to a section that is used to lock driver code
    PVOID    hDataSection;      // Handle to a section that is used to lock driver data
    TQueue   qMemList;          // Linked list of memory allocations

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


/******************************************************************************
*                                                                             *
*   Function prototypes                                                       *
*                                                                             *
******************************************************************************/

VOID
DpcVMRun(
    IN PKDPC          Dpc,
    IN PVOID          DeferredContext,
    IN PVOID          SystemArgument1,
    IN PVOID          SystemArgument2
    );


#endif //  _DEVICEEXTENSION_H_
