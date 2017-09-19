/******************************************************************************
*                                                                             *
*   Module:     Meta.c														  *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       4/12/2000													  *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the code related to meta data and its
        structures.

        Meta structures are associated with the physical pages of a virtual
        machine.  They contain pre-scanned clue-codes corresponding to
        the executable machine code, as well as a copy of the original code
        page with INT3 inserted over all the instructions that need to be
        virtualized.  When running a VM, we actually run this code page, not
        the original (when guest runs supervisor code).

        Meta structures are cached for performace, since the code pages are
        mostly reused.  This brings an interesting problem: when to invalidate?

        We can say that if a page is being written to, but not read, its meta
        data (if it exists) needs to be invalidated.  Who is writing to a page?
        (1) Code running within the VM can write to a meta-cached page
        (2) A virtual device may be writing to a meta-cached VM page
            (like floppy controller or dma virtual device)
        (3) Whoever else has an access to Mem.MemVM->linear may be writing
            to a meta-cached page

        We keep all pages that are cached Read-Only so we can detect any writes
        to them.


*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 4/12/2000	 Original                                             Goran Devic *
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

/******************************************************************************
*                                                                             *
*   void MetaInitialize()                                                     *
*                                                                             *
*******************************************************************************
*
*   Initializes meta data structure.
*
******************************************************************************/
void MetaInitialize()
{
    DWORD i;

    // Set up meta data pointers to make a doubly linked list

    Mem.pMetaHead = &Mem.Meta[0];
    Mem.pMetaTail = &Mem.Meta[MAX_META_DATA-1];
    Mem.Meta[0].prev = NULL;
    Mem.Meta[MAX_META_DATA-1].next = NULL;

    for( i=0; i<MAX_META_DATA-1; i++ )
        Mem.Meta[i].next = &Mem.Meta[i+1];

    for( i=MAX_META_DATA-1; i>0; i-- )
        Mem.Meta[i].prev = &Mem.Meta[i-1];
}


/******************************************************************************
*                                                                             *
*   TMeta * SetupMetaPage( DWORD dwVmCSIP )                                   *
*                                                                             *
*******************************************************************************
*
*   This function will set up and map a meta page that is
*   needed for execution of a page addressed by dwVmCSIP.
*
*   Where:
*       dwVmCSIP is the VM linear address of the CSIP to execute
*
*   Returns:
*       pointer to a meta structure describing current execution pages
*
******************************************************************************/
TMeta * SetupMetaPage( DWORD dwVmCSIP )
{
    TMeta *pMeta;
    TPage *pVmPte, *pMonPte;
    DWORD dwLinCSIP;

    // First, we need to find out if the target linear VM address is valid (mapped)
    pVmPte = VmLinearToVmPte( dwVmCSIP );
    if( pVmPte->fPresent==FALSE )
    {
        // Execution code page marked not present within the VM pages?
        // Send down page fault
        ASSERT(0);
    }

    ASSERT(pVmPte->Index < Mem.MemVM.pages);
    dwLinCSIP = Mem.MemVM.linear + 4096 * pVmPte->Index + (dwVmCSIP & 0xFFF);

    // Establish the mapping of the monitor page tables with the VM page tables
    pMonPte = BuildMonitorMapping( dwVmCSIP );

    // Find if the CECP physical address already have a meta-structure
    // in the cache, or we need to create a new one

    if( Mem.metaLookup[pVmPte->Index] != NULL )
    {
        // Good caching!  We have a meta-structure associated with this code page,
        // so the only work here is to make sure it is on the head of the LRU list

        pMeta = Mem.metaLookup[pVmPte->Index];

        if( pMeta->prev != NULL )
        {
            // Link it to the head of the LRU linked list of meta structures

            if( pMeta->next == NULL )                   // Last one?
            {                                           //  yes
                Mem.pMetaTail = pMeta->prev;
                pMeta->prev->next = NULL;
            }
            else
            {                                           //  no
                pMeta->next->prev = pMeta->prev;
                pMeta->prev->next = pMeta->next;
            }

            pMeta->prev = NULL;                         // we are first one now
            pMeta->next = Mem.pMetaHead;                // old first one..
            Mem.pMetaHead->prev = pMeta;                // link him to us
            Mem.pMetaHead = pMeta;                      // new head
        }

        ASSERT(pMeta->VmCodePage.Index==pVmPte->Index);
        ASSERT(pMeta->VmCodePage.fPresent==TRUE);
        ASSERT(pMeta->pPTE);
        ASSERT(pMeta->pPTE==pMonPte);
        ASSERT(pMonPte->fWrite==FALSE);
        pMeta->pPTE = pMonPte;
    }
    else
    {
        // This page does not have a meta-data associated with it, so create one
        // We use LRU and take a meta structure from the tail, link it to the head

        pMeta = Mem.pMetaTail;

        // If the LRU held a valid descriptor, invalidate it and reuse meta structure

        if( pMeta->VmCodePage.fPresent == TRUE )
        {
            Mem.metaLookup[ pMeta->VmCodePage.Index ] = NULL;

            // Since this points to code page, we may need to revert back Read-Only bit
            ASSERT( Mem.pPageStamp[ pMeta->VmCodePage.Index ] & PAGESTAMP_META_MASK );
            Mem.pPageStamp[ pMeta->VmCodePage.Index ] &= ~PAGESTAMP_META_MASK;
            if( Mem.pPageStamp[ pMeta->VmCodePage.Index ]==0 )
                pMeta->pPTE->fWrite = TRUE;
        }

        // Link the tail one to the head of the meta linked list

        ASSERT(pMeta->next==NULL);

        Mem.pMetaTail = pMeta->prev;                // new tail is before us
        pMeta->prev->next = NULL;                   // make him tail

        pMeta->prev = NULL;                         // we are first one now
        pMeta->next = Mem.pMetaHead;                // old first one..
        Mem.pMetaHead->prev = pMeta;                // link him to us
        Mem.pMetaHead = pMeta;                      // new head

        // Copy the CECP into the ModCP held as the first page in meta structure

        RtlCopyMemory( (void *) pMeta->MemPage.linear, (void *) (dwLinCSIP & ~0xFFF), 4096 );

        // Clean Meta page to zeroes

        RtlFillMemory( (void *) (pMeta->MemPage.linear + 4096), 4096, 0);

        // Register it with the physical metaLookup array

        Mem.metaLookup[pVmPte->Index] = pMeta;

        pMeta->VmCodePage.Index = pVmPte->Index;
        pMeta->VmCodePage.fPresent = TRUE;

        // Get the monitor PTE describing a page
        pMeta->pPTE = pMonPte;
        ASSERT(pMeta->pPTE);

        // Set the address of the PTE that describes this frame and make it read-only
        // Ignoring the stamp since the page will be marked RO in any case...
        pMeta->pPTE->fWrite = FALSE;
        Mem.pPageStamp[ pMeta->VmCodePage.Index ] |= PAGESTAMP_META_MASK;
    }

    ASSERT(Mem.pMetaHead->prev==NULL);
    ASSERT(Mem.pMetaTail->next==NULL);

    //=========================================================================
    // Ok, now we have a meta page ready and placed on the head of the LRU list
    // We need to map pages now as follows:
    //      (it's on my whiteboard :-)
    //=========================================================================

    // Map CECP' (currently executing code page that gets modified) into the
    // monitor 4th page (MonCECP) by the virtue of monitor page table (1st page)

    pMonitor->MonitorPTE[ MONITOR_PTE_CECP ].Index = pMeta->MemPage.physical[0];

    // Map CECP' into the VM address space via VM PTE map (that spans 4Mb)

    pMonitor->VM_PTE = *pMeta->pPTE;
    pMeta->pPTE->Index = pMeta->MemPage.physical[0];

    // Map the VM PTE page containing that entry into the monitor 3rd page,
    // so it can be modified in a cross-task code jump

    pMonPte = &pMonitor->VMPD[ dwVmCSIP >> 22 ];

    pMonitor->MonitorPTE[ MONITOR_PTE_PTE_CECP ].Index = pMonPte->Index;

    // Store the offset of the PTE entry within the private mapping of CECP
    // so the page-not-present can be set in a cross-task code jump;
    // That is required for our TLB-trick.

    pMonitor->offPTE_CECP = 4096 * MONITOR_PTE_PTE_CECP + (((dwVmCSIP >> 12) & 0x3FF) << 2);

    return( pMeta );
}


/******************************************************************************
*                                                                             *
*   void MetaInvalidatePage(TMeta *pMeta)                                     *
*                                                                             *
*******************************************************************************
*
*   Invalidate given meta page.
*
*   Where:
*       pMeta is the address of a meta page to invalidate
*
******************************************************************************/
void MetaInvalidatePage(TMeta *pMeta)
{
    ASSERT(pMeta);
    ASSERT(Mem.metaLookup[ pMeta->VmCodePage.Index ]);
    ASSERT(pMeta->VmCodePage.fPresent==TRUE);

    // If this is the current page, invalidate monitor pointer to it
    if( pMeta==pMonitor->pMeta )
        pMonitor->pMeta = NULL;

    // Clearing the address of the meta structure from the lookup
    // deletes it from the cache. Then, we need to make it not valid.
    ASSERT(Mem.metaLookup[ pMeta->VmCodePage.Index ] != NULL);
    Mem.metaLookup[ pMeta->VmCodePage.Index ] = NULL;
    pMeta->VmCodePage.fPresent = FALSE;

    // Restore the write flags that a page had before
    ASSERT(pMeta->pPTE);
    ASSERT(pMeta->pPTE->fWrite==FALSE);

    ASSERT( Mem.pPageStamp[ pMeta->VmCodePage.Index ] & PAGESTAMP_META_MASK );
    Mem.pPageStamp[ pMeta->VmCodePage.Index ] &= ~PAGESTAMP_META_MASK;
    if( Mem.pPageStamp[ pMeta->VmCodePage.Index ]==0 )
        pMeta->pPTE->fWrite = TRUE;

    // Link this meta structure to the tail
    if( pMeta->next != NULL )       // If the structure is not already on the tail...
    {
        // Unlink it from wherever it happens to be
        if( pMeta->prev == NULL )
            Mem.pMetaHead = pMeta->next;   // This structure is currently on the head
        else
            pMeta->prev->next = pMeta->next;
        pMeta->next->prev = pMeta->prev;

        // Link it to the tail
        pMeta->next = NULL;
        pMeta->prev = Mem.pMetaTail;
        ASSERT(Mem.pMetaTail->next==NULL);
        Mem.pMetaTail->next = pMeta;
        Mem.pMetaTail = pMeta;
    }

#if DBG 
    pMeta->pPTE = NULL;
    // Clear meta page to known values so we know if we accidentally use it
    ASSERT(pMeta->MemPage.pages==2);
    memset( (PVOID) pMeta->MemPage.linear, 0xC1, 2 * 4096 );
#endif
}


/******************************************************************************
*                                                                             *
*   void MetaInvalidateAll()                                                  *
*                                                                             *
*******************************************************************************
*
*   Invalidates complete meta cache.
*
******************************************************************************/
void MetaInvalidateAll()
{
    TMeta *pCur, *pNext;

    pCur = Mem.pMetaHead;
    do
    {
        pNext = pCur->next;
        if( pCur->VmCodePage.fPresent )
            MetaInvalidatePage(pCur);
        pCur = pNext;
    }
    while( pCur != NULL );

#if DBG 
    {
        DWORD i;
        for( i=0; i<Mem.MemVM.pages; i++ )
        {
            ASSERT((Mem.pPageStamp[i] & PAGESTAMP_META_MASK)==0);
            ASSERT(Mem.metaLookup[i]==NULL);
        }
    }
#endif
}


/******************************************************************************
*                                                                             *
*   void MetaInvalidateCurrent()                                              *
*                                                                             *
*******************************************************************************
*
*   Invalidates only the current meta cache structure pointed to by
*   pMonitor->pMeta
*
******************************************************************************/
void MetaInvalidateCurrent()
{
    if( pMonitor->pMeta )
        MetaInvalidatePage(pMonitor->pMeta);
}


/******************************************************************************
*                                                                             *
*   void MetaInvalidateVmPhysAddress( DWORD dwVmPhys )                        *
*                                                                             *
*******************************************************************************
*
*   Invalidate a meta page that covers given VM physical address.  The address
*   may or may not be covered with a meta page.  Physical address is zero-based,
*   and tops at the end of VM physical memory.
*
*   Where:
*       dwVmPhys is the VM physical address touched by the memory access
*
*   Returns:
*       TRUE if meta page existed covering that address
*
******************************************************************************/
BOOL MetaInvalidateVmPhysAddress( DWORD dwVmPhys )
{
    TMeta *pMeta;

    // Make sure we are in the correct range
    ASSERT((dwVmPhys >> 12) < Mem.MemVM.pages);

    // Get the real physical address of the page
    pMeta = Mem.metaLookup[ dwVmPhys >> 12 ];
    if( pMeta )
    {
        MetaInvalidatePage(pMeta);
        return( TRUE );
    }
    return( FALSE );
}


/******************************************************************************
*                                                                             *
*   BOOL MetaConditionallyInvalidateCode( TMeta *pMeta, DWORD dwOffset )      *
*                                                                             *
*******************************************************************************
*
*   Given a meta page and the offset into it, search 4 bytes after the offset
*   and 16 bytes before the offset for the meta-data.  Then, if some found,
*   invalidate page and return TRUE.
*
*   Where:
*       pMeta is the meta page to search and possibly invalidate
*       dwOffset is the target offset
*
*   Returns:
*       TRUE if the meta page got invalidated
*       FALSE if the meta page did not get invalidated
*
******************************************************************************/
BOOL MetaConditionallyInvalidateCode( TMeta *pMeta, DWORD dwOffset )
{
    DWORD *ptr;
    ASSERT(pMeta);
    ASSERT(Mem.metaLookup[ pMeta->VmCodePage.Index ]);

    if( dwOffset < 16 )
        ptr = (DWORD *) (pMeta->MemPage.linear + 4096);
    else
    {
        if( dwOffset > (4096 - 4))
            ptr = (DWORD *) (pMeta->MemPage.linear + 4096 + 4096 - 20);
        else
            ptr = (DWORD *) (pMeta->MemPage.linear + 4096 + dwOffset - 16);
    }

    if( ptr[0] || ptr[1] || ptr[2] || ptr[3] || ptr[4] )
    {
        MetaInvalidatePage(pMeta);
        return( TRUE );
    }

    return( FALSE );
}
