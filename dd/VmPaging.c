/******************************************************************************
*                                                                             *
*   Module:     VmPaging.c													  *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       8/1/2000													  *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the code for functions dealing with the memory
        issues of a virtual machine, such as paging and handling of A20 line.

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 8/1/2000	 Original                                             Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Include Files                                                             *
******************************************************************************/
#include <strmini.h>                // Include driver level C functions

#include <ntddk.h>                  // Include ddk function header file

#include "Vm.h"                     // Include root header file

#include "DisassemblerInterface.h"  // Include disassembler interface header

/******************************************************************************
*                                                                             *
*   Global Variables                                                          *
*                                                                             *
******************************************************************************/

extern TPage * post_fn_arg;

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
void CpuPageFaultUserCode();

/******************************************************************************
*                                                                             *
*   void A20SetState(BOOL fEnabled)                                           *
*                                                                             *
*******************************************************************************
*   This functions enables or disables the A20 line.
*
*   TODO: Messing with the A20 line has an effect of remapping upper 64K of memory
*         so that may need to be implemented
*
*   Where:
*       fEnabled is the new enabled state.
******************************************************************************/
void A20SetState(BOOL fEnabled)
{
    // TODO: A20 line affects real memory mapping

    if( fEnabled )      // ENABLE A20
    {
        if( Mem.fA20==FALSE )   // It was disabled
        {
            Mem.fA20 = TRUE;
            INFO(("A20 line turned ON"));
        }
    }
    else                // DISABLE A20
    {
        if( Mem.fA20==TRUE )    // It was enabled
        {
            Mem.fA20 = FALSE;
            INFO(("A20 line turned OFF"));
        }
    }
}


/******************************************************************************
*                                                                             *
*  void EnablePaging()                                                        *
*                                                                             *
*******************************************************************************
*   This funcition is called when VM paging has been enabled by loading PG
*   bit of a CR0 register.
******************************************************************************/

extern void SysRegPlaceStamps();

void EnablePaging()
{
    CPU.pPD = (TPage *) (Mem.MemVM.linear + (CPU.cr3 & ~0xFFF));

    // Reset monitor page directory
    RtlZeroMemory(pMonitor->VMPD, sizeof(pMonitor->VMPD));

    // Reset monitor pTE lookup to zeroes
    RtlZeroMemory( pMonitor->pPTE, sizeof(pMonitor->pPTE) );

    // Reset CPU lookup to VM PTE and Monitor PTE pages
    RtlZeroMemory( CPU.pPTE, sizeof(CPU.pPTE));

    // Rebase monitor and inject its page into the monitor PD
    RebaseMonitor();

    // Initialize stamp array with the registered system regions
    SysRegPlaceStamps();
}


/******************************************************************************
*                                                                             *
*  void DisablePaging()                                                       *
*                                                                             *
*******************************************************************************
*   This funcition is called when VM paging has been disabled by unloading PG
*   bit of a CR0 register.
******************************************************************************/
void DisablePaging()
{
    TPage *pPTE;
    DWORD i;

    // =======================================================================
    // Map VM page directory to span the whole VM PTE region of 4Mb
    // =======================================================================

    pPTE = (TPage *) pMonitor->VMPD;

    for( i=0; i<1024; i++, pPTE++ )
    {
        pPTE->Index    = Mem.MemPTE.physical[ i ];
        pPTE->fPresent = TRUE;
        pPTE->fUser    = TRUE;
        pPTE->fWrite   = TRUE;
    }

    // =======================================================================
    // Map base user memory from VM linear 0 inside VM PTE tables - we need to
    // preserve the flags containing signatures of memory regions.
    // =======================================================================

    pPTE = (TPage*) Mem.MemPTE.linear;

    for( i=0; i < 1024 * 1024; i++, pPTE++ )
    {
        if( i < (int)Mem.MemVM.pages )
        {
            pPTE->Index = Mem.MemVM.physical[i];        // Physical memory
            pPTE->fUser = TRUE;
        }
        else
        {
            pPTE->Index = 0;                            // Nothing
            pPTE->fPresent = FALSE;
        }

//      pPTE->fWrite   = TRUE;      // TODO: reset other structures and caches
    }
}


/******************************************************************************
*                                                                             *
*  TPage *BuildMonitorMapping( DWORD dwVmLin )                                *
*                                                                             *
*******************************************************************************
*   Maps a VM page that contains the given linear address into the monitor
*   paging structures so it can be accessed by the running VM.  The corresponding
*   VM page tables are marked accessed.
*
*   Where:
*       dwVmLin is the VM linear address to map
*
*   Returns:
*       Pointer to monitor PTE describing the page
*       0 if the VM PD or VM PTE mark that page not present
*
******************************************************************************/
TPage *BuildMonitorMapping( DWORD dwVmLin )
{
    DWORD dwVmPD, dwVmPTE, i, page;
    TPage *pVmFramePTE, *pMonFramePTE;

    // If paging is disabled, the mapping is very simple
    if( PAGING_ENABLED )
    {
        // TODO: Do we need to check if it is present here? Or it will always be present.
        pVmFramePTE = VmLinearToVmPte( dwVmLin );
        if( pVmFramePTE->fPresent==TRUE )
        {
            dwVmPD  = dwVmLin >> 22;
            dwVmPTE = (dwVmLin >> 12) & 0x3FF;

            if( pMonitor->VMPD[dwVmPD].fPresent==FALSE )
            {
                // Build the monitor page directory entry
                page = VmPageIndexToNtPageIndex( (CPU.pPD + dwVmPD)->Index );

                // Search all mapped monitor PD entries to see if the
                // page frame has already been mapped to reuse it
                for( i=0; i<1024; i++)
                {
                    if( i != dwVmPD )
                    {
                        if( ((CPU.pPD + i)->Index == page)
                        &&  (pMonitor->VMPD[i].fPresent == TRUE))
                            break;
                    }
                }

                if( i==1024 )
                {
                    // This is the first mapping of a PTE page frame
                    i = dwVmPD;
                    // Mark all pages in the current frame not present
                    RtlZeroMemory((void *)(Mem.MemPTE.linear + 4096 * i), sizeof(TPage) * 1024);
                }

                pMonFramePTE = (TPage *)(Mem.MemPTE.linear + 4096 * i);

                // Link the PTE frame from the page directory
                pMonitor->VMPD[dwVmPD].Index = Mem.MemPTE.physical[dwVmPD];
                pMonitor->VMPD[dwVmPD].fPresent = TRUE;
                pMonitor->VMPD[dwVmPD].fWrite   = TRUE;
                pMonitor->VMPD[dwVmPD].fUser    = (CPU.pPD + dwVmPD)->fUser;
                pMonitor->pPTE[dwVmPD] = pMonFramePTE;
                pMonFramePTE += dwVmPTE;              // Advance to the PTE offset
            }
            else
                pMonFramePTE = pMonitor->pPTE[dwVmPD] + dwVmPTE;

            if( pMonFramePTE->fPresent==FALSE )
            {
                // Build the monitor page table frame entry
                *pMonFramePTE = *pVmFramePTE;       // Copy the permissions
                ASSERT(pMonFramePTE->Index < Mem.MemVM.pages);
                pMonFramePTE->Index = Mem.MemVM.physical[ pMonFramePTE->Index ];

                // Mark the page read-only since we need to track its Dirty bit
                // and also possible system region writes
                pMonFramePTE->fWrite = FALSE;
            }

            // Mark the VM page directory and page table entry accessed
            (CPU.pPD + dwVmPD)->fAccessed = TRUE;
            pVmFramePTE->fAccessed     = TRUE;

            return( pMonFramePTE );
        }
    }
    else
    {
        // Paging is disabled - use flat mapping from MonPTE
        return( (TPage *)(Mem.MemPTE.linear) + (dwVmLin >> 12) );
    }

    return( NULL );
}


//=============================================================================
// PAGE FAULT HANDLER WHEN PAGING IS OFF
// Guest can be in either Real or Protected mode
//=============================================================================
void CpuPageFaultPagingOff()
{
    DWORD dwVmLinear, i;
    BYTE *pTarget, bMeta;
    TPage *pPage;
    TMEM *pMem;

    dwVmLinear = CSIPtoVmLinear();
    bMeta = *(BYTE *)(pMonitor->pMeta->MemPage.linear + 4096 + (dwVmLinear & 0xFFF));

    pPage = &Mem.pPagePTE[ pMonitor->guestCR2 >> 12 ];

    // pMonitor->nErrorCode tells us:
    // [0] - if the fault was caused by not-present page (0) or page-level protection violation (1)
    // [1] - whether access causing a fault was a read (0) or write (1)
    // [2] - the guest was executing in supervisor mode (0) or user mode (1)

    if( pMonitor->nErrorCode & 1 )      // Page-level protection: write to a RO page
    {
        // Write access to a page that is present and marked read-only:
        //  * Attempt to write to a ROM page (ignore for now since we simulate shadow ROM)
        //  * Write to a page that is meta-cached, including current meta page

        ASSERT((pMonitor->nErrorCode & 2) != 0);        // Make sure it was a write
        ASSERT((pMonitor->guestCR2 >> 12) < pVM->dwVmMemorySize);

        if( Mem.metaLookup[ pMonitor->guestCR2 >> 12 ] )
        {
            MetaInvalidatePage( Mem.metaLookup[ pMonitor->guestCR2 >> 12 ] );
            return;
        }
    }
    else                                // Page not-present
    {
        // Access to a non-present page:
        //  * TLB-catch: Reading currently executing code page
        //  * Device memory
        //  * Access to empty memory range while counting memory

        //===============================================================
        // If the fault was caused by the access to the same code page...
        //===============================================================
        if( (pMonitor->guestCR2 & ~0xFFF)==(dwVmLinear & ~0xFFF) )
        {
            // Invalidate current meta page.
            // TODO: Invalidate meta page only if the faulting mem access hit scanned region
            CPU.fCurrentMetaInvalidate = TRUE;

            // Execute instruction single step, in which case we allow read/write
            // access to the same code page
            SetMainLoopState( stateStep, NULL );

            return;
        }

        //===============================================================
        // Instruction is accessing a registered memory region
        //===============================================================
        // Instruction causing a page fault on a memory region that
        // was registered by a virtual device can be virtualized.
        // If not, it can execute with the `standard' fake page mechanism.
        // Find out in which memory arena the fault happened
        // TODO: Speed optimization: Index field of a device memory PTE could
        //       index the Device[] memory since those pages are marked not
        //       present, so we can keep additional info in the PTE record
        for( pMem = &Device.MEM[0], i=0; i<Device.MemTop; i++, pMem++ )
        {
            if( (pMonitor->guestCR2 >= pMem->BaseAddress) &&
                (pMonitor->guestCR2 <= pMem->LastAddress) )
            {
                pTarget = (BYTE *)CSIPtoDriverLinear();
                if( DisVirtualize( pTarget, dwVmLinear, pMem ) == 0 )
                {
                    // Position eip to the next instruction, even if we did not pick
                    // up all its arguments within the virtualization code

                    // TODO: What if PF is caused by instruction that was set to be
                    // terminating since it spans a page boundary, so the length may
                    // not be computed corerctly????
                    // At this point here, it would have to be one of those that access
                    // registered regions.

                    pMonitor->guestTSS.eip += bMeta & 0x0F;
                }
                return;
            }
        }

        //===============================================================
        // Access of an empty memory range
        //===============================================================
        if( pMonitor->guestCR2 >= pVM->dwVmMemorySize )
        {
            // Ignore instruction
            pMonitor->guestTSS.eip += bMeta & 0x0F;

            return;
        }
    }
    ERROR(("Page fault case not handled"));
}


void fnWriteReProtect(void)
{
    ASSERT(post_fn_arg);
    ASSERT(((DWORD)post_fn_arg >= Mem.MemPTE.linear) && ((DWORD)post_fn_arg < (Mem.MemPTE.linear + 1024 * 1024 * 4)));
    ASSERT(post_fn_arg->fPresent == TRUE);
    ASSERT(post_fn_arg->fWrite == TRUE);

    post_fn_arg->fWrite = FALSE;
    post_fn_arg = NULL;
}


//=============================================================================
// PAGE FAULT HANDLER WHEN PAGING IS ON
// Also, that implies protected mode is on as well.
//=============================================================================
void CpuPageFaultPagingOn()
{
    BYTE *pTarget, *pVmCR2, bMeta;
    DWORD dwVmCSIPLinear, dwVmFaultPageIndex, i, dwFaultPD, dwFaultPTE, dwFaultOffset;
    TPage *pMonPteFault, *pVmPteFault;
    TMEM *pMem;
    TSysReg_Callback post_fn;

    // pMonitor->nErrorCode tells us:
    // [0] - if the fault was caused by not-present page (0) or page-level protection violation (1)
    // [1] - whether access causing a fault was a read (0) or write (1)
    // [2] - the guest was executing in supervisor mode (0) or user mode (1)

    // Is the page fault caused from V86 mode code or Ring 3 PM ?
    if( pMonitor->nErrorCode & 4 )
    {
        CpuPageFaultUserCode();
        return;
    }

    dwFaultPD = pMonitor->guestCR2 >> 22;
    dwFaultPTE = (pMonitor->guestCR2 >> 12) & 0x3FF;
    dwFaultOffset = pMonitor->guestCR2 & 0xFFF;

    dwVmCSIPLinear = CSIPtoVmLinear();
    bMeta = *(BYTE *)(pMonitor->pMeta->MemPage.linear + 4096 + (dwVmCSIPLinear & 0xFFF));

    pMonPteFault = BuildMonitorMapping(pMonitor->guestCR2);
    pVmPteFault  = VmLinearToVmPte(pMonitor->guestCR2);
    ASSERT(pVmPteFault);

    if( pMonitor->nErrorCode & 1 )      // Page-level protection: write to a RO page
    {
        // Write access to a page that is present, completely mapped through, and marked read-only:
        //  * Attempt to write to a ROM page
        //  * Write to a page with system region data on it
        //  * Write to a page that is meta-cached, including current meta page
        //  * First write to a page that has no system region data on it

        ASSERT((pMonitor->nErrorCode & 2)==2);        // Make sure it was a write
        ASSERT(pMonPteFault);
        ASSERT(pMonPteFault->fPresent==TRUE);
        ASSERT(pMonPteFault->fWrite==FALSE);
        ASSERT(pMonitor->VMPD[dwFaultPD].fPresent==TRUE);
        ASSERT(pMonitor->pPTE[dwFaultPD]);
        ASSERT(CPU.pPD[dwFaultPD].fPresent==TRUE);

        // It was a write - mark the corresponding VM PTE dirty
        pVmPteFault->fDirty = TRUE;

        //===============================================================
        // Attempt to write to a ROM page
        //===============================================================
        // How to detect a ROM page?

        //===============================================================
        // Write to a protected system region inside a page
        //===============================================================
        // We protect system data structures from writing to them (GDT/TSS/PD...)
        // On a write, we unprotect it, let it execute single step, and then
        // call a system structure handler to do whatever adjustment it feels like
        dwVmFaultPageIndex = pVmPteFault->Index;

        if( Mem.pPageStamp[ dwVmFaultPageIndex ]==0 )
        {
            //===============================================================
            // A new regular page with no system region or meta data
            //===============================================================
            // Enable the write to it and proceed

            pMonPteFault->fWrite = TRUE;

            return;
        }
        else
        {
            // This VM physical page contains either meta or system region

            //===============================================================
            // Write to a page that is in the meta-cache
            //===============================================================
            if( Mem.metaLookup[ dwVmFaultPageIndex ] )
            {
                // The page is marked RO since it may contain a system region as well

                MetaInvalidatePage( Mem.metaLookup[ dwVmFaultPageIndex ] );
            }

            post_fn = SysRegFind(pMonitor->guestCR2, dwVmFaultPageIndex);

            // In any case mark the page writable and execute instruction single step.
            if( post_fn != NULL )
            {
                // System region has been found - mark the page writable and
                // continue single step with the correct post function
                pMonPteFault->fWrite = TRUE;
                post_fn_arg = pMonPteFault;        // Save the argument for a post-function

                SetMainLoopState( stateStep, (TPostState) post_fn );
                return;
            }
            else
            {
                // No system region.  Mark the page writable, execute single step and
                // unmark it on the post callback

                pMonPteFault->fWrite = TRUE;
                post_fn_arg = pMonPteFault;        // Save the argument for a post-function

                SetMainLoopState( stateStep, fnWriteReProtect );
                return;
            }
        }
    }                                   //===================//
    else                                // Page not-present  //
    {                                   //===================//
        // Access to a non-present page:
        // I  VM marks the address present
        //      * TLB-catch: Reading currently executing code page
        //      * First-time accessing a valid page
        // II VM marks the address not present
        //      * Device memory
        //      * Real Page Fault (Access to a not present page as mapped by the VM)

        if( pMonPteFault != NULL )
        {
            // Page was not present, but now it is mapped into the monitor paging, so the
            // VM had it present...
            //===============================================================
            // If the fault was caused by the access to the same code page...
            //===============================================================
            if( (pMonitor->guestCR2 & ~0xFFF)==(dwVmCSIPLinear & ~0xFFF) )
            {
                // Invalidate current meta page only if we wrote to it.
                // TODO: Invalidate meta page only if the faulting mem access hit scanned region
                if( pMonitor->nErrorCode & 2 )
                    MetaInvalidateCurrent();

                // Execute instruction single step, in which case we allow read/write
                // access to the same code page
                SetMainLoopState( stateStep, NULL );

                return;
            }

            //===============================================================
            // First time accessing a valid page
            //===============================================================
            return;
        }
        // We could not map the page into the monitor paging structure:
        // Either VM PD or VM PTE marks the address not present.  It can still
        // be device memory... Before we send PF down.

        //===============================================================
        // Instruction is accessing a registered memory region
        //===============================================================
        // Instruction causing a page fault on a memory region that
        // was registered by a virtual device can be virtualized.
        // If not, it can execute with the `standard' fake page mechanism.
        // Find out in which memory arena the fault happened
        // TODO: Speed optimization: Index field of a device memory PTE could
        //       index the Device[] memory since those pages are marked not
        //       present, so we can keep additional info in the PTE record
        for( pMem = &Device.MEM[0], i=0; i<Device.MemTop; i++, pMem++ )
        {
            if( (pMonitor->guestCR2 >= pMem->BaseAddress) &&
                (pMonitor->guestCR2 <= pMem->LastAddress) )
            {
                pTarget = (BYTE *)CSIPtoDriverLinear();
                if( DisVirtualize( pTarget, dwVmCSIPLinear, pMem ) == 0 )
                {
                    // Position eip to the next instruction, even if we did not pick
                    // up all its arguments within the virtualization code

                    // TODO: What if PF is caused by instruction that was set to be
                    // terminating since it spans a page boundary, so the length may
                    // not be computed corerctly????
                    // At this point here, it would have to be one of those that access
                    // registered regions.

                    pMonitor->guestTSS.eip += bMeta & 0x0F;
                }
                return;
            }
        }

        //===============================================================
        // VM PD or PTE marked address not present
        //===============================================================

        CPU_Exception( EXCEPTION_PAGE_FAULT, pMonitor->nErrorCode );
        return;
    }
    ERROR(("Page fault case not handled"));
}


//=============================================================================
// PAGE FAULT HANDLER FOR PF IN USER MODE
// Page fault from V86 mode or Ring 3 Protected Mode
//=============================================================================
void CpuPageFaultUserCode()
{
    BYTE *pTarget;
    TPage *pMonPteFault, *pVmPteFault;
    TMEM *pMem;
    DWORD i;

    pVmPteFault  = VmLinearToVmPte(pMonitor->guestCR2);
    pMonPteFault = BuildMonitorMapping(pMonitor->guestCR2);

    ASSERT(pVmPteFault);

    if( pMonitor->nErrorCode & 1 )     //========================//
    {                                  // Page-level protection  //
                                       //========================//
        // Write access to a page that is present, completely mapped through, and marked read-only
        // or marked as having only supervisor access:
        //  * Write to a Read-Only page
        //  * Write to a Supervisor-level page
        //
        ASSERT(0);

        //===============================================================
        // VM PD or PTE marked address not present
        //===============================================================

        CPU_Exception( EXCEPTION_PAGE_FAULT, pMonitor->nErrorCode );
        return;
    }                                   //===================//
    else                                // Page not-present  //
    {                                   //===================//
        // Access to a non-present page:
        // I  VM marks the address present
        //      * First-time accessing a valid page
        // II VM marks the address not present
        //      * Device memory
        //      * Real Page Fault (Access to a not present page as mapped by the VM)

        if( pMonPteFault != NULL )
        {
            //===============================================================
            // We just mapped a new page so we can proceed
            //===============================================================
            return;
        }
        else
        {
            //===============================================================
            // Instruction is accessing a registered memory region
            //===============================================================
            // Instruction causing a page fault on a memory region that
            // was registered by a virtual device can be virtualized.
            // If not, it can execute with the `standard' fake page mechanism.
            // Find out in which memory arena the fault happened
            // TODO: Speed optimization: Index field of a device memory PTE could
            //       index the Device[] memory since those pages are marked not
            //       present, so we can keep additional info in the PTE record

            //===============================================================
            // VM PD or PTE marked address not present
            //===============================================================

            CPU_Exception( EXCEPTION_PAGE_FAULT, pMonitor->nErrorCode );
            return;
        }

        ASSERT(0);
    }
}
