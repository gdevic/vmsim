/******************************************************************************
*                                                                             *
*   Module:     MainLoop.c                                                    *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       3/14/2000                                                     *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the code for the monitor cross-jump and
        main loop that runs the virtual machine.

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 3/14/2000  Original                                             Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Include Files                                                             *
******************************************************************************/
#include <strmini.h>                // Include driver level C functions

#include <ntddk.h>                  // Include ddk function header file

#include "Vm.h"                     // Include root header file

#include "ioctl.h"                  // Include IO control defines

#include "DeviceExtension.h"        // Include deviceExtension

/******************************************************************************
*                                                                             *
*   Global Variables                                                          *
*                                                                             *
******************************************************************************/

// Aid to debugging

DWORD run = 0;
DWORD stopat = 0;
DWORD fSingleStep = 0;
DWORD fSilent = 0;
DWORD state = stateNext;
TPostState fn_PostState = NULL;
TPage * post_fn_arg = NULL;
WORD xcs = 0xFFFF;
DWORD xeip = 0x0;
DWORD fMetaAll = 0;

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
*   void SetMainLoopState( int new_state, TPostState funct )                  *
*                                                                             *
*******************************************************************************
*
*   This function sets the state of the main loop execution.  Optionally,
*   a function may be provided that will be called after a single step
*   operation.
*
*   Where:
*       new_state is one of the stateNext, stateStep, stateIoCtl
*       funct is the optional function for post-stateStep
*
******************************************************************************/
void SetMainLoopState( int new_state, TPostState funct )
{
    state = new_state;
    fn_PostState = funct;
}


/******************************************************************************
*                                                                             *
*   void RunVM()                                                              *
*                                                                             *
*******************************************************************************
*
*   This function is called via IOCTL to start running the VM
*
******************************************************************************/
void RunVM(DWORD param)
{
    PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)param;
    DWORD dwVmCSIP;
    DWORD dwPhysCSIP;
    static fFirstTime = TRUE;

#if DBG
    if( fFirstTime )
    {
//        _asm int 3;
        fSilent = 1;
        fFirstTime = FALSE;
    }
#endif

    //-----------------------------------------------------------------------------
    // We are running in a separate thread of execution, so we need to periodically
    // check for exit status (pDevExt->runState)
    //-----------------------------------------------------------------------------
    while( 1 )
    {
#if DBG
        if( ++run==stopat ) Breakpoint; // Help to debug: stop at certain run loop number
        if( (xcs==pMonitor->guestTSS.cs) && (xeip==pMonitor->guestTSS.eip) )
            _asm int 3;
        if( fSingleStep )   state = stateStep;
        if( run > 0xe51400 )
            fSilent = 0;
#endif
        switch( state )
        {
            //---------------------------------------
            // Synchronous IoControl in progress...
            //---------------------------------------
            case stateIoCtl:
                return;

            //---------------------------------------
            // Normal scan-before-execute execution
            //---------------------------------------
            case stateNext:

                if (pDevExt->runState == vmStateStopRequested || pDevExt->runState == vmStateSuspended)
                    return;

                // See if we got a signal to reset the CPU

                if( CPU.fResetPending )
                {
                    CPU_Reset();                        // Reset the virtual CPU
                    CPU.fMetaInvalidate = TRUE;         // Signal to invalidate meta pages
                    CPU.fCurrentMetaInvalidate = FALSE; // No need to invalidate current page now
                    SetMainLoopState(stateNext, NULL);  // back to the initial state
                }

                // Check for invalidation of meta pages request

                if( CPU.fMetaInvalidate || fMetaAll )
                {
                    CPU.fMetaInvalidate = FALSE;
                    MetaInvalidateAll();
                }
                else
                if( CPU.fCurrentMetaInvalidate )
                {
                    CPU.fCurrentMetaInvalidate = FALSE;
                    MetaInvalidateCurrent();
                }

                // Check if we have an external hardware interrupt to process

                if( CPU.fInterrupt && (CPU.eflags & IF_MASK))
                {
                    STATS_INC(cPushInt);
                    CPU_Interrupt( Pic_Ack() );
                }

                // Get the linear address of the VM code to run

                dwVmCSIP =  CSIPtoVmLinear();

                // Prepare meta-data for the code run

                pMonitor->pMeta = SetupMetaPage( dwVmCSIP );

                // Set the VM page address for a jump, just to load I TLB entry

                pMonitor->fn_CECP = dwVmCSIP & ~0xFFF;

                // Scan the code page from the CSIP address forth

                ScanCodePage(
                    (BYTE *)(pMonitor->pMeta->MemPage.linear + (dwVmCSIP & 0xFFF)), // CSIP pointer
                    (BYTE *)(pMonitor->pMeta->MemPage.linear + 4096),               // Meta page base address
                    0 );                                                            // 16-bit code & data

                TaskSwitch();           // Run the virtual machine

                // Restore mapping of the meta page within the VM PTE to original page

                *pMonitor->pMeta->pPTE = pMonitor->VM_PTE;
#if DBG
                if( (xcs==pMonitor->guestTSS.cs) && (xeip==pMonitor->guestTSS.eip) )
                    _asm int 3;
#endif
            break;

            //---------------------------------------
            // Execute instruction single-step
            //---------------------------------------
            case stateStep:
                    pMonitor->guestTSS.eflags |= TF_MASK;

                    TaskSwitch();           // Run the virtual machine single step

                    // Call the post-loop function (if defined)
                    if( fn_PostState != NULL )
                        (*fn_PostState)();
                break;

            //---------------------------------------
            // Native V86 or Ring-3 PM code
            //---------------------------------------
            case stateNative:
                    pMonitor->guestTSS.eflags |= INTERNAL_RUN_NATIVE_MASK;

                    TaskSwitch();           // Run the virtual machine in native mode

                    // Call the post-loop function (if defined)
                    if( fn_PostState != NULL )
                        (*fn_PostState)();
                break;

            default:        // Huh?
                ASSERT(0);
        }

        STATS_INC(cMainLoop);

        // And see what happened...

        if( pMonitor->nInterrupt >= 32 )
        {
            // Interrupts 32-255 are caused by external (not CPU) controller, so we will
            // execute appropriate INT n instruction

            STATS_INC(cExternalInt);
            STATS_EXTINT(pMonitor->nInterrupt);
            ExecuteINTn( pMonitor->nInterrupt );
        }
        else
        {
            // Ok, we are dealing with a CPU fault/trap;
            // TODO: We really dont need to pass pMonitor->pMeta since who needs it can always get it from there
            STATS_INC(cCpuInt);
            ExecuteTrap( pMonitor->pMeta, pMonitor->nInterrupt );
        }
    }
}


