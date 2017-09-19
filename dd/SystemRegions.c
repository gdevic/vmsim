/******************************************************************************
*                                                                             *
*   Module:     SystemRegions.c                                               *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       8/14/2000                                                     *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the code for handling the VM system memory
        regions and data structures such as TSS, GDT, LDT, IDT and PD/PTE.

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 8/14/2000  Original                                             Goran Devic *
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

extern TPage * post_fn_arg;

/******************************************************************************
*                                                                             *
*   Local Defines, Variables and Macros                                       *
*                                                                             *
******************************************************************************/
#define MAX_PAGES           3       // A system region consists of so many physical pages
#define MAX_MAPPINGS        4       // A page is mapped to no more than that many virtual addresses

#define MAX_PTES            32      // Keeping track of so many PTEs


typedef struct TagPte
{
    struct TagPte *next;            // Next structure in a list

    DWORD VmPage;                   // Index of a physical page that this describes
    TPage *pMonPTE[MAX_MAPPINGS];   // Pointer to multiple mappings of that page

} TPte;

typedef struct TagRegion
{
    DWORD dwVmLinStart;             // Primary mapping starting address
    DWORD dwVmLinEnd;               // Primary mapping ending address (inclusive)
    DWORD nPages;                   // How many physical pages a region spans
    DWORD VmPage[MAX_PAGES];        // Array of physical pages that this describes
    TPage *pMonPTE[MAX_MAPPINGS];   // Pointer to monitor PTEs describing first page

} TRegion;


typedef struct
{
    TPte pte[MAX_PTES];             // Array of available pte pagers
    TPte *pPteAvail;                // Available chain

    TPte *pPte;                     // Linked list of pte pagers
    TRegion m[SYSREG_MAX - 1];      // Array of multi-pagers:
    //  TRegion SYSREG_TSS              structure holding a TSS
    //  TRegion SYSREG_GDT              structure holding a GDT
    //  TRegion SYSREG_LDT              structure holding a LDT
    //  TRegion SYSREG_IDT              structure holding a IDT
    //  TRegion SYSREG_PD               structure holding a PD
    //  SYSREG_PTE                      (not used)

} TSys;

TSys Sys;

/******************************************************************************
*                                                                             *
*   Functions                                                                 *
*                                                                             *
******************************************************************************/
extern void Handler_TSS(void);
extern void Handler_GDT(void);
extern void Handler_LDT(void);
extern void Handler_IDT(void);
extern void Handler_PD (void);
extern void Handler_PTE(void);

static TSysReg_Callback static_callbacks[SYSREG_MAX] =
{ Handler_TSS, Handler_GDT, Handler_LDT, Handler_IDT, Handler_PD, Handler_PTE };

/******************************************************************************
*                                                                             *
*   void InitSysReg()                                                         *
*                                                                             *
*******************************************************************************
*
*   Initializes system regions subsystem.
*
******************************************************************************/
void InitSysReg()
{
    DWORD i;

    RtlZeroMemory((void *) &Sys, sizeof(TSys));

    // Initialize linked list of available single pagers
    for( i=0; i<MAX_PTES - 1; i++ )
        Sys.pte[i].next = &Sys.pte[i+1];
    Sys.pPteAvail = &Sys.pte[0];
}


void SysRegPlaceStamps()
{
    TRegion *pRegion;
    DWORD index, pages;
ASSERT(0);
    ASSERT(NOT(PAGING_ENABLED));

    // Traverse through the pPTE records and put stamps
    // The same with regions (m) arrays

    for( index=SYSREG_TSS; index < SYSREG_PD; index++ )
    {
        pRegion = &Sys.m[index];
        ASSERT(pRegion->pMonPTE[0]==NULL);

        for( pages=0; pages<pRegion->nPages; pages++ )
        {
            ASSERT(pRegion->VmPage[pages] < Mem.MemVM.pages);
            Mem.pPageStamp[pRegion->VmPage[pages]] |= 1 << index;
        }
    }
}


/******************************************************************************
*   VM tried to write to its TSS.
*
*   We just let it write
******************************************************************************/
void Handler_TSS(void)
{
    ASSERT(0);

    // Re-disable page that we just wrote to
    post_fn_arg->fWrite = FALSE;
}


/******************************************************************************
*   VM tried to write to its GDT.
*
*   We just let it write
******************************************************************************/
void Handler_GDT(void)
{
    DWORD dwSel;
    ASSERT(0);

    // Calculate which selector was written
    dwSel = pMonitor->guestCR2 - CPU.guestGDT.base;

    // Copy new descriptor and fix up privilege level
    TranslateSingleVmGdtToMonitorGdt( dwSel );

    // Re-disable page that we just wrote to
    post_fn_arg->fWrite = FALSE;
}


/******************************************************************************
*   VM tried to write to its LDT.
*
*   We just let it write
******************************************************************************/
void Handler_LDT(void)
{
    ASSERT(0);

    // Re-disable page that we just wrote to
    post_fn_arg->fWrite = FALSE;
}


/******************************************************************************
*   VM tried to write to its IDT.
*
*   We just let it write
******************************************************************************/
void Handler_IDT(void)
{
    ASSERT(0);

    // Re-disable page that we just wrote to
    post_fn_arg->fWrite = FALSE;
}


/******************************************************************************
*   VM tried to write to its PD.
******************************************************************************/
void Handler_PD(void)
{
    ASSERT(0);

    // Re-disable page that we just wrote to
    post_fn_arg->fWrite = FALSE;
}


/******************************************************************************
*   VM tried to write to one of its PTE
******************************************************************************/
void Handler_PTE(void)
{
    ASSERT(0);

    // Re-disable page that we just wrote to
    post_fn_arg->fWrite = FALSE;
}


/******************************************************************************
*                                                                             *
*   TSysReg_Callback SysRegFind( DWORD dwVmLin, DWORD dwVmPage )              *
*                                                                             *
*******************************************************************************
*   Given a VM access address, returns whether it addresses a registered
*   system region.
*
*   Where:
*       dwVmLin is the VM linear access address
*       dwVmPage is the VM physical page
*
*   Returns:
*       Address to a handler function
*       NULL if the address dwVmPhys is not in a set of system regions
******************************************************************************/
TSysReg_Callback SysRegFind( DWORD dwVmLin, DWORD dwVmPage )
{
    TPte *pPte;
    TRegion *pRegion;
    DWORD index, page, offset, phys_page;

    // Quick search over the regions for primary registration
    for( index=SYSREG_TSS; index<SYSREG_PTE; index++ )
    {
        if((Sys.m[index].dwVmLinStart <= dwVmLin)
         &&(Sys.m[index].dwVmLinEnd   >= dwVmLin))
        {
            return( static_callbacks[index] );
        }
    }
#if 0
    // Break the target address into VM page number and offset within it
    page = dwVmPhys >> 12;
    offset = dwVmPhys & 0xFFF;

    // Quick lookup the stamp to see if somebody registered a page
    ASSERT(page < Mem.MemVM.pages);

    // TODO: What if we dont use stamps?
//    if( (Mem.pPageStamp[page] & ~PAGESTAMP_META_MASK) != 0 )
    {
        // Search a linked list of single pagers (Page Table frames)
        pPte = Sys.pPte;
        while( pPte != NULL )
        {
            if( pPte->VmPage==page )
                return( static_callbacks[SYSREG_PTE] );
            pPte = pPte->next;
        }

        // Search the multi-pagers
        for( index = SYSREG_TSS; index < SYSREG_MAX - 1; index++)
        {
            pRegion = Sys.m[index];
            for( phys_page=0; phys_page < pRegion->nPages; phys_page++ )
            {
                // TODO: More granularity for checking if a region is hit,
                //       right now we do only a whole page
                if( pRegion->VmPage[phys_page]==page )
                    return( static_callbacks[index] );
            }
        }

        // Stamp was there, but the region has not been found...
        ERROR(("Stamp but no region!"));
    }
#endif
    return( NULL );
}


/******************************************************************************
*                                                                             *
*   void RenewSystemRegion( int eSysReg, DWORD dwVmAddress, DWORD dwLen )     *
*                                                                             *
*******************************************************************************
*   Adds a system region to the list.  This function deletes whatever was
*   registered under the specified system region before adding a new definition.
*   To add a system region to a multiple PTE list, use AddSystemRegionPTE()
*
*   Where:
*       eSysReg is one of the SYSREG_* regions
*       dwVmAddress is the VM LINEAR address of the region
*       dwLen is the total length in bytes of the region
*
******************************************************************************/
void RenewSystemRegion( int eSysReg, DWORD dwVmAddress, DWORD dwLen )
{
    TRegion *pRegion;
    TPage *pPte, *pMonPte;
    int iLen;
    DWORD pages;
    ASSERT(eSysReg < SYSREG_PTE);

    // First delete that system region
    DeleteSystemRegion( eSysReg );

    if( dwLen != 0 )
    {
        pRegion = &Sys.m[eSysReg];

        // Add the common information about the region
        pRegion->dwVmLinStart = dwVmAddress;
        pRegion->dwVmLinEnd   = dwVmAddress + dwLen - 1;

        iLen = (int) dwLen;

        for( pages=0; iLen > 0; pages++, iLen-=4096, dwVmAddress += 4096 )
        {
            pPte = VmLinearToVmPte(dwVmAddress);
            ASSERT(pPte);
            ASSERT(pPte->fPresent==TRUE);
            pRegion->VmPage[pages] = pPte->Index;

            if( PAGING_ENABLED )
            {
                Mem.pPageStamp[pPte->Index] |= 1 << eSysReg;
                pMonPte = BuildMonitorMapping( dwVmAddress );
                ASSERT(pMonPte);
                ASSERT(pMonPte->fPresent==TRUE);

                // Make it read-only from the master PTE
                pMonPte->fWrite = FALSE;

                // Store the pointer to the monitor pte into the sysreg structure
                pRegion->pMonPTE[pages] = pMonPte;
                pRegion->pMonPTE[pages+1] = NULL;
            }
        }

        pRegion->nPages = pages;
    }
    else
        INFO(("System region length is 0"));
}


/******************************************************************************
*                                                                             *
*   void DeleteSystemRegion( int eSysReg )                                    *
*                                                                             *
*******************************************************************************
*   Deletes a system region from the list.  Use this function to delete
*   all regions except SYSREG_PTE since it deletes all entries.
*   (A PTE can have multiple independent regions.)
*
*   Where:
*       eSysReg is one of the SYSREG_* regions
*
******************************************************************************/
void DeleteSystemRegion( int eSysReg )
{
    TRegion *pRegion;
    TPage *pPte;
    DWORD index, span;
    BYTE stamp;
    ASSERT(eSysReg < SYSREG_PTE);

    pRegion = &Sys.m[eSysReg];

    // If paging is enabled, we need to reset corresponding page table entries' RO attributes
    if( PAGING_ENABLED )
    {
        stamp = 1 << eSysReg;
        index = 0;

        // Reset the page stamps
        for( span=0; span < pRegion->nPages; span++ )
        {
            ASSERT((Mem.pPageStamp[ pRegion->VmPage[index] ] & stamp) != 0 );
            Mem.pPageStamp[pRegion->VmPage[index]] &= ~stamp;
        }

        while( (pPte = pRegion->pMonPTE[index]) != NULL )
        {
            for( span=0; span < pRegion->nPages; span++ )
            {
                ASSERT(pPte->fWrite==FALSE);
                if( Mem.pPageStamp[ pRegion->VmPage[span] ]==0 )
                    pPte->fWrite = TRUE;
            }
            index++;
        }
    }

    // Invalidate the structure itself
    pRegion->dwVmLinStart = 0;
    pRegion->dwVmLinEnd = 0;
    pRegion->nPages = 0;
    pRegion->pMonPTE[0] = NULL;
#if DBG
    RtlZeroMemory((void *)pRegion->VmPage,  sizeof(DWORD) * MAX_PAGES);
    RtlZeroMemory((void *)pRegion->pMonPTE, sizeof(TPage *) * MAX_MAPPINGS);
#endif
}


/******************************************************************************
*                                                                             *
*   void AddSystemRegionPTE( DWORD dwVmPhys )                                 *
*                                                                             *
*******************************************************************************
*   Adds a PTE region to the list of registered PTE regions.
*
*   Where:
*       dwVmPhys is the physical address of the PTE frame to add.
*
******************************************************************************/
void AddSystemRegionPTE( DWORD dwPhys )
{
#if 0
    TSysReg *p;
ASSERT(0);

    p = Sys.pSysRegAvail;
    if( p==NULL )
        ERROR(("Run out of sysreg structures.. Increase MAX_SYSREG and recompile"));
    Sys.pSysRegAvail = p->next;

    // Link the new node to the head of the PTE linked list
    p->next = Sys.p[ SYSREG_PTE ];
    Sys.p[ SYSREG_PTE ] = p;

    // Set up the content of the new node
    p->VmPage        = dwPhys >> 12;
    p->dwStartOffset = 0;
    p->dwEndOffset   = 4095;
    p->pMonPTE       = NULL;
    p->callback      = Handler_PTE;

    // Add a page stamp
    // TODO: Who is marking the page read-only??
    ASSERT(p->VmPage < Mem.MemVM.pages);
    ASSERT((Mem.pPageStamp[p->VmPage] &= 1 << SYSREG_PTE)==0);

    Mem.pPageStamp[p->VmPage] |= 1 << SYSREG_PTE;
#endif
}


/******************************************************************************
*                                                                             *
*   void DeleteSystemRegionPTE( DWORD dwVmPhys )                              *
*                                                                             *
*******************************************************************************
*   Deletes a PTE system region specified by the base VM physical address.
*
*   Where:
*       dwVmPhys is the VM physical address of the PTE to delete
*
******************************************************************************/
void DeleteSystemRegionPTE( DWORD dwVmPhys )
{
#if 0
    TSysReg *p;
    DWORD page;
ASSERT(0);

    if( Sys.p[SYSREG_PTE] != NULL )
    {
        p = Sys.p[ SYSREG_PTE ];
        page = dwVmPhys >> 12;
        ASSERT(page < Mem.MemVM.pages);

        // Search the linked list of PTE region descriptors for the one to delete
        while( p != NULL )
        {
            ASSERT((Mem.pPageStamp[ p->VmPage ] & (1<<SYSREG_PTE)) != 0 );
            ASSERT(p->pMonPTE->fWrite==FALSE);

            // If the base VM physical address match, delete a single entry
            if( p->VmPage == page )
            {
                // Delete a mark and make page writable if there are no more marks on it
                Mem.pPageStamp[ p->VmPage ] &= ~(1<<SYSREG_PTE);
                if( Mem.pPageStamp[ p->VmPage ]==0 )
                    p->pMonPTE->fWrite = TRUE;

                return;
            }
        }
    }
#endif
}

