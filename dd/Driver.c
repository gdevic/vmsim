/******************************************************************************
*                                                                             *
*   Module:     Driver.c                                                      *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       3/9/2000                                                      *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the main driver interface code.

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 3/9/2000   Original                                             Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Include Files                                                             *
******************************************************************************/
#include <ntddk.h>                  // Include ddk function header file

#include "Ioctl.h"                  // Include I/O control defines

#include "Vm.h"                     // Include root header file

#include "Queue.h"                  // Include some queue functions

#include "DeviceExtension.h"        // Include deviceExtension

/******************************************************************************
*                                                                             *
*   Local Defines, Variables and Macros                                       *
*                                                                             *
******************************************************************************/
// Extract transfer type

#define IOCTL_TRANSFER_TYPE( _iocontrol)   (_iocontrol & 0x3)

/******************************************************************************
*                                                                             *
*   Global Variables                                                          *
*                                                                             *
******************************************************************************/

PDEVICE_OBJECT GUIDevice;       // Our user-inteface device object

//-----------------------------------------------------------------------------
// Global driver data pertaining the VM operation
//-----------------------------------------------------------------------------

TVM      *pVM;                  // Pointer to VM data structure
TMonitor *pMonitor;             // Pointer to Monitor data structure

TMemory   Mem;                  // Driver memory management structure
TCPU      CPU;                  // Virtual CPU data structure
TDevice   Device;               // Virtual devices structures


/******************************************************************************
*                                                                             *
*   Functions                                                                 *
*                                                                             *
******************************************************************************/

//======================================================================
//         D E V I C E - D R I V E R  R O U T I N E S
//======================================================================

/******************************************************************************
*                                                                             *
*   void OutputMessageToGUI()                                                 *
*                                                                             *
*******************************************************************************
*
*   This function is to use the RequestIrp to have the GUI put a message out
*
******************************************************************************/
void OutputMessageToGUI(PDEVICE_EXTENSION pDevExt, char *mssg)
{
    static VMREQUEST VmReq;

    VmReq.head.type = ReqVMMessage;
    VmReq.head.bufferlen = strlen(mssg);
    VmReq.head.pComplete = NULL;
    RtlMoveMemory(VmReq.buffer, mssg, VmReq.head.bufferlen);

    SendRequestToGui(&VmReq);
}


/******************************************************************************
*                                                                             *
*   void SendRequestToGui(PVMREQUEST pVmReq)                                  *
*                                                                             *
*******************************************************************************
*
*   Forms a request message and sends it up to the GUI application via
*   a spare irp.  The request may not be sent immediately, depending on the
*   availability of an irp; this function queues it and returns.
*
*   Where:
*       pVmReq is the address of the request structure to be sent where:
*         pVmReq->type is the request type
*         pVmReq->bufferlen is the memory size of the buffer to transfer
*
*   Returns:
*       void
*
******************************************************************************/
void SendRequestToGui(PVMREQUEST pVmReq)
{
    PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)GUIDevice->DeviceExtension;
    PIO_STACK_LOCATION irpStack;
    PIRP pIrp;
    KIRQL irql;

    ASSERT(pDevExt);
    ASSERT(pVmReq);
    ASSERT(pVmReq->head.bufferlen<=MAX_TRANSFER_BYTES);

    KeAcquireSpinLock(&pDevExt->SpinLockReq, &irql);

    if( pDevExt->VMRequestIrp != 0 )
    {
        // An irp is available to send the request up to the app

        pIrp = pDevExt->VMRequestIrp;
        irpStack = IoGetCurrentIrpStackLocation (pIrp);
        ASSERT(irpStack->Parameters.DeviceIoControl.OutputBufferLength == sizeof(VMREQUEST));

        RtlMoveMemory(pDevExt->pVmRequest, pVmReq, sizeof(VMREQUEST_HEADER)+pVmReq->head.bufferlen);
        pDevExt->pVmRequest->head.requestID = pDevExt->requestID++;

        // Close the irp and send it up

        pIrp->IoStatus.Information = sizeof(VMREQUEST_HEADER)+pVmReq->head.bufferlen;
        pIrp->IoStatus.Status = STATUS_SUCCESS;
        pDevExt->VMRequestIrp = NULL;
        IoCompleteRequest( pIrp, IO_NO_INCREMENT );

        KeReleaseSpinLock(&pDevExt->SpinLockReq, irql);
    }
    else
    {
        // There is currently no available irp.. Queue the request.

        void *p = ExAllocatePool(NonPagedPool, sizeof(VMREQUEST_HEADER)+pVmReq->head.bufferlen);
        if( p )
        {
            RtlMoveMemory(p, pVmReq, sizeof(VMREQUEST_HEADER)+pVmReq->head.bufferlen);
            if( QEnqueue( pDevExt->qVmReq, p )==0 )
                ERROR(("Can't queue"));
        }
        else
            ERROR(("Can't allocate memory"));

        KeReleaseSpinLock(&pDevExt->SpinLockReq, irql);
    }
}


//----------------------------------------------------------------------
//
// VmDeviceControl
//
//----------------------------------------------------------------------
BOOLEAN  VmDeviceControl( IN PIO_STACK_LOCATION irpStack,
        IN PFILE_OBJECT FileObject, IN BOOLEAN Wait,
        IN PVOID InputBuffer, IN ULONG InputBufferLength,
        OUT PVOID OutputBuffer, IN ULONG OutputBufferLength,
        IN ULONG IoControlCode, OUT PIO_STATUS_BLOCK IoStatus,
        IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
    PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)GUIDevice->DeviceExtension;
    PVMINPUTDEVICE pVmInput;
    int dev;

    _try
    {
#if 0
        INFO(("DeviceIOControl:"));
        INFO(("  FileObject         %08X", (DWORD)FileObject));
        INFO(("  Wait               %08X", (DWORD)Wait));
        INFO(("  InputBuffer        %08X", (DWORD)InputBuffer));
        INFO(("  InputBufferLength  %08X", (DWORD)InputBufferLength));
        INFO(("  OutputBuffer       %08X", (DWORD)OutputBuffer));
        INFO(("  OutputBufferLength %08X", (DWORD)OutputBufferLength));
        INFO(("  IoControlCode      %08X", (DWORD)IoControlCode));
        INFO(("  IoStatus           %08X", (DWORD)IoStatus));
        INFO(("  DeviceObject       %08X", (DWORD)DeviceObject));
#endif
        // Presume invalid parameter; when they are checked, at the end of
        // each case, we set it to successful if it is so.  Also presume that
        // nothing is returned.

        IoStatus->Status = STATUS_INVALID_PARAMETER;
        IoStatus->Information = 0;

        switch( IoControlCode )
        {
            //====================================================================
            case VMSIM_AllocTVM:
            //====================================================================

                INFO(("Code: VMSIM_AllocTVM"));

                // Allocate shared system virtual address and map it to user space
                //
                // Use this call to allocate a TVM structure only since the system
                // address will be assigned to pVM within a driver context.
                //
                // Expects:
                //  OutputBuffer       -> valid buffer to strore the address
                //  OutputBufferLength -> 4
                //
                // Will set:
                //  OutputBuffer       <- address of the buffer mapped in user space
                //                        NULL if allocation not successful
                // This call ignores 'nb' parameter of DeviceIoControl()

                if((OutputBuffer != NULL)
                && (OutputBufferLength == 4))
                {
                    *(int *)OutputBuffer = 0;       // Presume failure

                    if( pVM==NULL )
                    {
                        // Allocate memory using our memory tracking function

                        pVM = (TVM *) MemoryAlloc(sizeof(TVM), OutputBuffer, TRUE);

                        if( pVM )
                        {
                            IoStatus->Status = STATUS_SUCCESS;
                            IoStatus->Information = 4;
                        }
                        else
                        {
                            ERROR(("Failed to allocate VM memory"));
                            IoStatus->Status = STATUS_MEMORY_NOT_ALLOCATED;
                        }
                    }
                    else
                    {
                        ERROR(("VM already allocated"));
                        IoStatus->Status = STATUS_MEMORY_NOT_ALLOCATED;
                    }
                }
                break;

            //====================================================================
            case VMSIM_FreeTVM:
            //====================================================================

                INFO(("Code: VMSIM_FreeTVM"));

                // Free memory that was allocated using VMSIM_AllocTVM call
                // It is ok to try to try to free a NULL pointer.
                //
                // Expects:
                //  InputBuffer        -> address in user space of the TVM structure
                //  InputBufferLength  -> sizeof(TVM)
                // This call ignores 'nb' parameter of DeviceIoControl()

                if((InputBuffer != NULL)
                && (InputBufferLength == sizeof(TVM)))
                {
                    IoStatus->Status = STATUS_SUCCESS;

                    if( *(DWORD *)InputBuffer == 0 )
                    {
                        INFO(("Freeing NULL pointer, but here that's OK"));
                        break;
                    }

                    // This code causes a generic shut-down call.  It will shut down
                    // a VM and free all the memory associated with it.

                    ShutDownVM();
                }
                break;

            //====================================================================
            case VMSIM_MapROM:
            //====================================================================

                INFO(("Code: VMSIM_MapROM"));

                // Maps ROM image supplied by the user app into the virtual machine
                // This function is marked as METHOD_IN_DIRECT.  The (large) buffer
                // is passed in the output buffer, and the small control structure
                // is passed in the input buffer.  This structure contains the
                // buffer len and the address to map it to.
                //
                // Expects:
                //  InputBuffer        -> Control structure
                //  InputBufferLength  -> DWORD:[size][address]
                //  OutputBuffer       -> address of a ROM image
                //  OutputBufferLength -> image size
                // This call ignores 'nb' parameter of DeviceIoControl()

                if((InputBuffer != NULL)
                && (InputBufferLength == 2 * sizeof(DWORD))
                && (OutputBuffer != NULL))
                {
                    if( pVM )
                    {
                        if( pMonitor )
                        {
                            void *pROM = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, HighPagePriority);
                            ASSERT(pROM);

                            MemoryMapROM( pROM,                 // ROM image address
                                *((DWORD *)InputBuffer+0),      // ROM size
                                *((DWORD *)InputBuffer+1) );    // Where to map (VM physical)

                            IoStatus->Status = STATUS_SUCCESS;
                        }
                        else
                            ERROR(("VM not initialized"));
                    }
                    else
                        ERROR(("VM not allocated"));
                }
                break;

            //====================================================================
            case VMSIM_MemCheck:
            //====================================================================

                INFO(("Code: VMSIM_MemCheck"));

                // Returns the check code from memory tracker checking
                // Check the memory lists and store the resulting code
                //
                // Expects:
                //  OutputBuffer       -> valid buffer
                //  OutputBufferLength -> 4
                //
                // Will set:
                //  OutputBuffer       <- QCheck return code
                // This call ignores 'nb' parameter of DeviceIoControl()

                if((OutputBuffer != NULL)
                && (OutputBufferLength == 4))
                {
                    IoStatus->Status = STATUS_SUCCESS;
                    *(int *)OutputBuffer = MemoryCheck();

                    INFO(("MemoryCheck() .. returning code %d\n",*(int *)OutputBuffer));
                }
                break;

            //====================================================================
            case VMSIM_FindNTHole:
            //====================================================================

                INFO(("Code: VMSIM_FindNTHole"));

                // Finds the memory address in the host NT5 that is safe for initial
                // monitor cross-page
                // Search NT5 page directory page, from the top down, for a page table
                // entry that is not present
                //
                // Expects:
                //  OutputBuffer       -> valid buffer
                //  OutputBufferLength -> 4
                // This call ignores 'nb' parameter of DeviceIoControl()

                if((OutputBuffer != NULL)
                && (OutputBufferLength == 4))
                {
                    TPage *pPage = (TPage *) (0xC0300000 + 0x1000 - 4);
                    DWORD dwAddress = 0xFFC00000;

                    *(int *)OutputBuffer = 0;       // Presume failure

                    for( ; dwAddress!=0; pPage--, dwAddress -= 0x400000)
                    {
                        if( pPage->fPresent==0 )
                        {
                            IoStatus->Status = STATUS_SUCCESS;
                            *(int *)OutputBuffer = dwAddress;

                            INFO(("VMSIM_FindNTHole found at %08X\n", dwAddress ));

                            break;
                        }
                    }
                }
                break;

            //====================================================================
            case VMSIM_InitVM:
            //====================================================================

                INFO(("Code: VMSIM_InitVM"));

                // Initializes a virtual machine data shared structures
                //
                // Expects:
                //  OutputBuffer       -> address of the result variable
                //  OutputBufferLength -> 4
                // This call ignores 'nb' parameter of DeviceIoControl()

                if((OutputBuffer != NULL)
                && (OutputBufferLength == 4))
                {
                    if( pVM )
                    {
                        if( (*(DWORD *)OutputBuffer = InitVM()) == 0)
                        {
                            IoStatus->Status = STATUS_SUCCESS;
                            IoStatus->Information = 4;
                        }
                        else
                            ERROR(("VMSIM_InitVM: InitVM() failed"));
                    }
                    else
                        ERROR(("VMSIM_InitVM: VM not allocated"));
                }
                break;

            //====================================================================
            case VMSIM_RunVM:
            //====================================================================

                INFO(("Code: VMSIM_RunVM"));

                // Gives a virtual machine a chance to run as far as it can go without
                // the support of a driver
                //
                // Expects:
                //  InputBuffer        -> address of the VM structure
                //  InputBufferLength  -> sizeof(TVM)
                //  OutputBuffer       -> address of the vm request structure
                //  OutputBufferLength -> sizeof(VMREQUEST)
                //
                // Returns:
                //  OutputBuffer       <- Sets the request structure

                if((InputBuffer != NULL)
                && (InputBufferLength == sizeof(TVM))
                && (OutputBuffer != NULL)
                && (OutputBufferLength == sizeof(VMREQUEST)))
                {
                    // Store the request structure into our global to be accessible
                    // This one is a synchronous request irp

                    pDevExt->pSyncVmReq = (VMREQUEST *) OutputBuffer;

                    // If we are in the suspended state, we keep app. RunVM() thread
                    // alive by bouncing back and forth and sleeping in the app

                    RunVM((DWORD) pDevExt);

                    if(pDevExt->runState == vmStateSuspended)
                        pDevExt->pSyncVmReq->head.type = ReqVMSuspended;

                    // Set the return length for the `nb' DeviceIoControl's parameter

                    IoStatus->Status = STATUS_SUCCESS;
                    IoStatus->Information = OutputBufferLength;
                }
                break;

            //====================================================================
            case VMSIM_GetVMRequest:
            //====================================================================

                INFO(("Code: VMSIM_GetVMRequest"));

                // Gives a virtual machine a method to pass async requests up to gui
                //
                // Expects:
                //  OutputBufferLength -> sizeof(VMREQUEST)
                // This buffer is passed on using METHOD_OUT_DIRECT, so we need to
                // map system memory from mdl to get its address
                //
                // Returns:
                //  OutputBuffer       <- Sets the request structure

                if(OutputBufferLength == sizeof(VMREQUEST))
                {
                    KIRQL irql;

                    KeAcquireSpinLock(&pDevExt->SpinLockReq, &irql);
                    ASSERT(pDevExt->VMRequestIrp==0);

                    // Get the address of the request structure including the buffer
                    // and map it in the system memory for use by the driver.
                    // Note: the buffer should be marked as METHOD_OUT_DIRECT so it is locked
                    // Store the request structure so it is accessible.

                    pDevExt->pVmRequest = (PVMREQUEST) MmGetSystemAddressForMdlSafe(Irp->MdlAddress, HighPagePriority);
                    pDevExt->VMRequestIrp = Irp;

                    // If there is a VM request pending, send it up.  If not, retain
                    // the irp for later

                    if( QIsNotEmpty(pDevExt->qVmReq) )
                    {
                        PVMREQUEST p;

                        // A request is available to send it up to the app

                        p = QDequeue(pDevExt->qVmReq);

                        RtlMoveMemory(  pDevExt->pVmRequest,
                                        p,
                                        sizeof(VMREQUEST_HEADER)+p->head.bufferlen);
                        pDevExt->pVmRequest->head.requestID = pDevExt->requestID++;

                        Irp->IoStatus.Information = sizeof(VMREQUEST_HEADER)+p->head.bufferlen;
                        Irp->IoStatus.Status = STATUS_SUCCESS;
                        pDevExt->VMRequestIrp = NULL;
                        IoCompleteRequest( Irp, IO_NO_INCREMENT );

                        KeReleaseSpinLock(&pDevExt->SpinLockReq, irql);

                        return FALSE;
                    }
                    else
                    {
                        // There is currently no request.  Retain the irp.

                        IoStatus->Status = STATUS_PENDING;
                        IoMarkIrpPending(Irp);

                        KeReleaseSpinLock(&pDevExt->SpinLockReq, irql);

                        return FALSE;
                    }
                }
                else
                    ASSERT(0);
                break;

            //====================================================================
            case VMSIM_CompleteVMRequest:
            //====================================================================

                INFO(("Code: VMSIM_CompleteVMRequest"));

                // Call by the app to complete a driver request, both synchronous
                // and asynchronous.
                //
                // Expects:
                //  InputBuffer        -> address of the original VMREQUEST structure (buffered)
                //  InputBufferLength  -> sizeof(VMREQUEST)
                //  OutputBufferLength -> size of the additional buffer (optional)
                //      This buffer is passed on using METHOD_IN_DIRECT, so we need to
                //      map system memory from mdl to get to it, if it is used.

                if((InputBuffer != NULL)
                && (InputBufferLength == sizeof(VMREQUEST)))
                {
                    BYTE *pBuffer = NULL;
                    PVMREQUEST pVmReq = (PVMREQUEST) InputBuffer;

                    // If the extra optional buffer is being used, map it in the system memory
                    if( OutputBufferLength > 0 )
                    {
                        pBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, HighPagePriority);
                        ASSERT(pBuffer);
                    }

                    Irp->IoStatus.Status = STATUS_SUCCESS;
                    IoStatus->Information = sizeof(VMREQUEST);

                    if( pVmReq->head.pComplete )
                        (pVmReq->head.pComplete)(pVmReq, pVmReq->head.completeArg, pBuffer);
                }
                else
                    ASSERT(0);
                break;

            //====================================================================
            case VMSIM_Control:
            //====================================================================

                INFO(("Code: VMSIM_Control"));

                // Immediate control call from the app down to the driver
                //
                // Expects:
                //  InputBuffer        -> address of the vm control structure
                //  InputBufferLength  -> sizeof(VMCONTROL)
                // This call ignores 'nb' parameter of DeviceIoControl()

                if((InputBuffer != NULL)
                && (InputBufferLength == sizeof(VMCONTROL)))
                {
                    PVMCONTROL pVmCtrl = (PVMCONTROL)InputBuffer;

                    IoStatus->Status = STATUS_SUCCESS;

                    switch (pVmCtrl->type)
                    {
                    case CtlStartVM:
                        INFO(("Control: CtlStartVM"));

                        // Set the machine state to running
                        pDevExt->runState = vmStateRunning;

                        // Send the start message to all virtual devices

                        for( dev=0; dev < (int)Device.VddTop; dev++ )
                        {
                            if( Device.Vdd[dev].Start != NULL )
                            {
                                INFO(("Starting virtual %s", Device.Vdd[dev].Name));
                                (Device.Vdd[dev].Start)();
                            }
                        }
                        break;

                    case CtlStopVM:
                        INFO(("Control: CtlStopVM"));

                        // Set a flag to stop the machine
                        // and complete all outstanding Irps
                        pDevExt->runState = vmStateStopRequested;
                        break;

                    case CtlSuspendVM:
                        INFO(("CtlSuspendVM"));

                        ASSERT(pDevExt->runState == vmStateRunning);

                        // Set the machine state to suspended
                        pDevExt->runState = vmStateSuspended;

                        // Send the suspend message to all virtual devices

                        for( dev=0; dev < (int)Device.VddTop; dev++ )
                        {
                            if( Device.Vdd[dev].Suspend != NULL )
                            {
                                INFO(("Suspending virtual %s", Device.Vdd[dev].Name));
                                (Device.Vdd[dev].Suspend)();
                            }
                        }
                        break;

                    case CtlResumeVM:
                        INFO(("CtlResumeVM"));

                        ASSERT(pDevExt->runState == vmStateSuspended);

                        // Set the machine state to running again
                        pDevExt->runState = vmStateRunning;

                        // Send the resume message to all virtual devices

                        for( dev=Device.VddTop-1; dev>=0; dev-- )
                        {
                            if( Device.Vdd[dev].Resume != NULL )
                            {
                                INFO(("Resuming virtual %s", Device.Vdd[dev].Name));
                                (Device.Vdd[dev].Resume)();
                            }
                        }
                        break;

                    case CtlResetVM:
                        INFO(("CtlResetVM"));
                        ERROR(("Not yet implemented"));
                        break;

                    case CtlAltDelVM:
                        INFO(("CtlAltDelVM"));
                        ERROR(("Not yet implemented"));
                        break;

                    default:
                        ERROR(("Invalid parameter"));
                        IoStatus->Status = STATUS_INVALID_PARAMETER;
                    }
                }
                break;

            //====================================================================
            case VMSIM_InputDevice:
            //====================================================================

                INFO(("Code: VMSIM_InputDevice"));

                // Application calls this method to pass an input to a simulator:
                // Keyboard or mouse input.
                //
                // Expects:
                //  InputBuffer        -> address of the VMINPUTDEVCE structure
                //  InputBufferLength  -> sizeof(VMINPUTDEVCE)

                if((InputBuffer != NULL)
                && (InputBufferLength == sizeof(VMINPUTDEVICE)))
                {
                    pVmInput = (PVMINPUTDEVICE) InputBuffer;

                    IoStatus->Status = STATUS_SUCCESS;

                    switch( pVmInput->type )
                    {
                        case InKeyPress:    // Enque raw scancodes into the keyboard buffer
                                do
                                {
                                    ControllerEnq((BYTE)(pVmInput->scan_code & 0xFF), SOURCE_KEYBOARD);
                                    pVmInput->scan_code >>= 8;
                                }
                                while((pVmInput->scan_code & 0xFF) != 0);

                            break;

                        default:            // Not yet handled
                            ASSERT(0);
                    }
                }
                break;

            //====================================================================
            case VMSIM_CleanItUp:
            //====================================================================

                INFO(("Code: VMSIM_CleanItUp"));
                // bad previous exit - clean up IRPs

                IoStatus->Status = STATUS_SUCCESS;
                pDevExt->runState = vmStateStopped;
                if (pDevExt->VMRequestIrp)
                {
                    pDevExt->VMRequestIrp->IoStatus.Status = STATUS_CANCELLED;
                    IoCompleteRequest( pDevExt->VMRequestIrp, IO_NO_INCREMENT );
                    pDevExt->VMRequestIrp = NULL;
                }
                break;

            //====================================================================
            case VMSIM_RegisterMemory:
            //====================================================================

                INFO(("Code: RegisterMemory"));
                ASSERT(0);
                break;

            //====================================================================
            case VMSIM_RegisterIO:
            //====================================================================

                INFO(("Code: VMSIM_RegisterIO"));
                ASSERT(0);
                break;

            //====================================================================
            case VMSIM_RegisterPCI:
            //====================================================================

                INFO(("Code: VMSIM_RegisterPCI"));
                ASSERT(0);
                break;

            //====================================================================
            case VMSIM_GenerateIRQ:
            //====================================================================

                INFO(("Code: VMSIM_GenerateIRQ"));

                // Inserts an interrupt request into a PIC
                //
                // Expects:
                //  InputBuffer        -> valid buffer containing IRQ number
                //  InputBufferLength  -> 4
                //  OutputBuffer       -> valid buffer
                //  OutputBufferLength -> 4
                //
                // Will set:
                //  OutputBuffer       <- 0 (ok)
                //                        1 (bad IRQ number)
                // This call ignores 'nb' parameter of DeviceIoControl()

                if((InputBuffer != NULL)
                && (InputBufferLength == 4)
                && (OutputBuffer != NULL)
                && (OutputBufferLength == 4))
                {
                    *(int *)OutputBuffer = VmSIMGenerateInterrupt(*(DWORD *)InputBuffer);

                    IoStatus->Status = STATUS_SUCCESS;
                    IoStatus->Information = 4;
                }
                break;

            //====================================================================
            default:
            //====================================================================
                ERROR(("Code: unknown code of %08X\n",IoControlCode));
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                break;
        }
    }
    _except(EXCEPTION_EXECUTE_HANDLER)
    {
        ERROR(("Exception in VmDeviceControl"));
        IoStatus->Status = STATUS_ACCESS_DENIED;

        return(FALSE);
    }

    return TRUE;
}


//----------------------------------------------------------------------
//
// VmDispatch
//
// In this routine we handle requests to our own device. The only
// requests we care about handling explicitely are IOCTL commands that
// we will get from the GUI. We also expect to get Create and Close
// commands when the GUI opens and closes communications with us.
//
//----------------------------------------------------------------------
NTSTATUS VmDispatch( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
    PIO_STACK_LOCATION      irpStack;
    PVOID                   inputBuffer;
    PVOID                   outputBuffer;
    ULONG                   inputBufferLength;
    ULONG                   outputBufferLength;
    ULONG                   ioControlCode;
    BOOLEAN                 completeTheIrp = TRUE;
    PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)GUIDevice->DeviceExtension;

    BreakPoint;

    INFO(("VmDispatch()"));

    // Go ahead and set the request up as successful
    //
    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    //
    // Get a pointer to the current location in the Irp. This is where
    //     the function codes and parameters are located.
    //
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    //
    // Get the pointer to the input/output buffer and its length
    //
    inputBuffer             = Irp->AssociatedIrp.SystemBuffer;
    outputBuffer            = Irp->AssociatedIrp.SystemBuffer;

    inputBufferLength       = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength      = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    ioControlCode           = irpStack->Parameters.DeviceIoControl.IoControlCode;

    switch (irpStack->MajorFunction)
    {
        case IRP_MJ_CREATE:
            INFO(("VmCSim: IRP_MJ_CREATE"));

            break;

        case IRP_MJ_CLOSE:
            INFO(("VmCSim: IRP_MJ_CLOSE"));

            // Cancel all known timers and complete irps
            if (pDevExt->VMRunIrp)
            {
                pDevExt->VMRunIrp->IoStatus.Status = STATUS_CANCELLED;
                IoCompleteRequest( pDevExt->VMRunIrp, IO_NO_INCREMENT );
                pDevExt->VMRunIrp = NULL;
                pDevExt->runState = vmStateStopped;
            }
            if (pDevExt->VMRequestIrp)
            {
                pDevExt->VMRequestIrp->IoStatus.Status = STATUS_CANCELLED;
                IoCompleteRequest( pDevExt->VMRequestIrp, IO_NO_INCREMENT );
                pDevExt->VMRequestIrp = NULL;
            }
            break;

        case IRP_MJ_DEVICE_CONTROL:
            INFO (("VmCSim: IRP_MJ_DEVICE_CONTROL"));

            // See if the output buffer is really a user buffer that we
            // can just dump data into.

            if( IOCTL_TRANSFER_TYPE(ioControlCode) == METHOD_NEITHER )
            {
                inputBuffer  = irpStack->Parameters.DeviceIoControl.Type3InputBuffer;
                outputBuffer = Irp->UserBuffer;
            }

            //
            // Its a request from the GUI
            //
            completeTheIrp =
                VmDeviceControl( irpStack, irpStack->FileObject, TRUE,
                         inputBuffer, inputBufferLength,
                         outputBuffer, outputBufferLength,
                         ioControlCode, &Irp->IoStatus, DeviceObject, Irp );
            break;
    }

    if (completeTheIrp)
        IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return STATUS_SUCCESS;
}


//----------------------------------------------------------------------
//
// VmUnload
//
// Our job is done - time to leave.
//
//----------------------------------------------------------------------
VOID VmUnload( IN PDRIVER_OBJECT DriverObject )
{
    WCHAR deviceLinkBuffer[]  = L"\\DosDevices\\VmCSim";
    UNICODE_STRING deviceLinkUnicodeString;
    PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)GUIDevice->DeviceExtension;

    INFO(("VmCSim: VmUnload()"));

    // Unlock the driver code and data

    MmUnlockPagableImageSection(pDevExt->hCodeSection);
    MmUnlockPagableImageSection(pDevExt->hDataSection);

    // Now we can free any memory blocks we have outstanding
    _try
    {
        QDestroyAlloc( pDevExt->qVmReq );
        ShutDownVM();
    }
    _except(EXCEPTION_EXECUTE_HANDLER)
    {
        ERROR(("Exception in VmUnload"));
    }

    // Delete the symbolic link for our device

    RtlInitUnicodeString( &deviceLinkUnicodeString, deviceLinkBuffer );
    IoDeleteSymbolicLink( &deviceLinkUnicodeString );

    // Delete the device object

    IoDeleteDevice( DriverObject->DeviceObject );
}


//----------------------------------------------------------------------
//
// DriverEntry
//
// Installable driver initialization. Here we just set ourselves up.
//
//----------------------------------------------------------------------
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath )
{
    NTSTATUS                ntStatus;
    WCHAR                   deviceNameBuffer[]  = L"\\Device\\VmCSim";
    UNICODE_STRING          deviceNameUnicodeString;
    WCHAR                   deviceLinkBuffer[]  = L"\\DosDevices\\VmCSim";
    UNICODE_STRING          deviceLinkUnicodeString;
    PETHREAD                curthread;
    int                     i;

    BreakPoint;

    INFO(("DriverEntry()"));

    // Setup our name

    RtlInitUnicodeString (&deviceNameUnicodeString, deviceNameBuffer );

    // Set up the device used for GUI communications

    ntStatus = IoCreateDevice ( DriverObject,
                   sizeof(DEVICE_EXTENSION),
                   &deviceNameUnicodeString,
                   FILE_DEVICE_VMSIM,
                   0,
                   TRUE,
                   &GUIDevice );
    if (NT_SUCCESS(ntStatus))
    {
        _try
        {
            PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)GUIDevice->DeviceExtension;
            // initialize DeviceExtension
            pDevExt->runState = vmStateStopped;
            pDevExt->VMRequestIrp = NULL;

            // Create a symbolic link that the GUI can specify to gain access
            // to this driver/device

            RtlInitUnicodeString (&deviceLinkUnicodeString, deviceLinkBuffer );
            ntStatus = IoCreateSymbolicLink (&deviceLinkUnicodeString,
                         &deviceNameUnicodeString );
            if (!NT_SUCCESS(ntStatus))
                ERROR(("IoCreateSymbolicLink failed"));

            // Create dispatch points for all routines that must be handled

            DriverObject->MajorFunction[IRP_MJ_CREATE]          =
            DriverObject->MajorFunction[IRP_MJ_CLOSE]           =
            DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = VmDispatch;
            DriverObject->DriverUnload                          = VmUnload;

            //===================================================================
            // Lock the driver code and data in memory

            pDevExt->hCodeSection = MmLockPagableCodeSection((PVOID)&pDevExt->hCodeSection);
            MmLockPagableSectionByHandle(pDevExt->hCodeSection);

            pDevExt->hDataSection = MmLockPagableDataSection((PVOID)&pDevExt->hDataSection);
            MmLockPagableSectionByHandle(pDevExt->hDataSection);

            // Clear and reset the globals

            pVM = NULL;
            pMonitor = NULL;
#if DBG 
            RtlFillMemory(&Mem,    sizeof(TMemory), 0x55);
            RtlFillMemory(&CPU,    sizeof(TCPU),    0x55);
            RtlFillMemory(&Device, sizeof(TDevice), 0x55);
#endif
            //===================================================================
            // Now we will set up memory management list, so that we minimize
            // memory leaks during development

            MemoryAllocInit();

            KeInitializeSpinLock(&pDevExt->SpinLockReq);
            pDevExt->qVmReq = QCreateAlloc(NonPagedPool);

        }
        _except(EXCEPTION_EXECUTE_HANDLER)
        {
            ERROR(("Exception in DriverEntry"));
            ntStatus = !STATUS_SUCCESS;
        }
    }

    if (!NT_SUCCESS(ntStatus))
    {
        ERROR(("Failed to create device"));

        // Something went wrong, so clean up (free resources etc)

        if( GUIDevice )
            IoDeleteDevice( GUIDevice );
    }

    return ntStatus;
}

