/******************************************************************************
*                                                                             *
*   Module:     Translations.c                                                *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       9/6/2000                                                      *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the code for various address translations:

DWORD CSIPtoVmLinear()

    This function returns linear address within the runnig VM of where its
    CS:(E)IP points to.  The address depends on segment / selector base.

DWORD CSIPtoDriverLinear()

    This function returns driver mapping of a VM's CS:(E)IP pointer.  Mode
    of execution as well as paging is taken into account.

DWORD VmLinearToDriverLinear( DWORD) dwVmLin )

    Returns driver mapping for a VM virtual address.  If the paging is on,
    it uses VM page table translations to try to get the VM address that in
    turn maps to driver address.

TPage * VmLinearToVmPte( DWORD dwVmLin )

    Given the VM linear address, return the address of the VM page table entry
    describing it.  If the paging is off, that is a simple 1-1 mapping, and
    since VM does not have page tables set up, a pointer to a internal static
    page is returned.  If the paging is on, we perform translation using
    VM page tables.

DWORD VmPageIndexToNtPageIndex( DWORD VmIndex )

    Given a page number of a virtual page inside the VM, returns the NT's
    effective index of that page within the host OS.  That index is suitable
    to use within monitor PD and PTE pages.

DWORD AnyNtVirtualToPhysical( DWORD dwAddress )

    Given a virtual address (anywhere in the driver), returns the physical page
    number.  The address have to be locked.

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 9/6/2000   Original                                             Goran Devic *
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
*   DWORD CSIPtoVmLinear()                                                    *
*                                                                             *
*******************************************************************************
*   This function returns linear address within the runnig VM of where its
*   CS:(E)IP points to.  The address depends on segment / selector base.
*
*   Returns:
*       linear address of the CS:EIP in the VM address space
******************************************************************************/
DWORD CSIPtoVmLinear()
{
    DWORD Base;

    if( pMonitor->guestTSS.eflags & VM_MASK )
    {
        // CS contains segment value (running either `real` mode or v86 mode)
        ASSERT((pMonitor->guestTSS.eip>>16)==0);
        return( ((DWORD)pMonitor->guestTSS.cs << 4) + pMonitor->guestTSS.eip );
    }
    else
    {
        // CS contains selector into a descriptor table: it can be GDT or LDT
        if( pMonitor->guestTSS.cs & SEL_TI_MASK )
        {
            // Local descriptor table
            ASSERT(0);
        }
        else
        {
            // Global descriptor table
            Base = GetGDTBase(&pMonitor->GDT[ pMonitor->guestTSS.cs >> 3 ]);
        }

        return( Base + pMonitor->guestTSS.eip );
    }
}

/******************************************************************************
*                                                                             *
*   DWORD CSIPtoDriverLinear()                                                *
*                                                                             *
*******************************************************************************
*   This function returns driver mapping of a VM's CS:(E)IP pointer.  Mode
*   of execution as well as paging is taken into account.
*
*   Returns:
*       linear address mapped in a driver of the location
******************************************************************************/
DWORD CSIPtoDriverLinear()
{
    DWORD VmLinear;
    DWORD HostLinear;

    VmLinear   = CSIPtoVmLinear();
    HostLinear = VmLinearToDriverLinear(VmLinear);
    ASSERT(HostLinear);

    return( HostLinear );
}


/******************************************************************************
*                                                                             *
*   DWORD VmLinearToDriverLinear( DWORD) dwVmLin )                            *
*                                                                             *
*******************************************************************************
*   Returns driver mapping for a VM virtual address.  If the paging is on,
*   it uses VM page table translations to try to get the VM address that in
*   turn maps to driver address.
*
*   Where:
*       dwVmLin is the virtual address in the VM address space
*   Returns:
*       Driver address corresponding to the given virtual address
*       0 if the paging is on and the address is not mapped,
*       0 if the paging is off and the address is outside physical VM memory
******************************************************************************/
DWORD VmLinearToDriverLinear( DWORD dwVmLin )
{
    TPage *pPte;
    DWORD dwAddress;

    if( PAGING_ENABLED )
    {
        pPte = VmLinearToVmPte(dwVmLin);
        ASSERT(pPte);

        if( pPte->fPresent==TRUE )
        {
            ASSERT(pPte->Index < Mem.MemVM.pages);
            dwAddress = Mem.MemVM.linear + 4096 * pPte->Index;

            return( dwAddress + (dwVmLin & 0xFFF) );
        }

    }
    else    // PAGING DISABLED
    {
        if( dwVmLin < pVM->dwVmMemorySize )
            return( Mem.MemVM.linear + dwVmLin );
    }

    // Defaulting to page not present - return 0
    return( 0 );
}


/******************************************************************************
*                                                                             *
*  TPage * VmLinearToVmPte( DWORD dwVmLin )                                   *
*                                                                             *
*******************************************************************************
*   Given the VM linear address, return the address of the VM page table entry
*   describing it.  If the paging is off, that is a simple 1-1 mapping, and
*   since VM does not have page tables set up, a pointer to a internal static
*   page is returned.  If the paging is on, we perform translation using
*   VM page tables.
*
*   Where:
*       dwVmLin is the VM linear address
*
*   Returns:
*       Pointer to a TPage structure with .index field having a VM page number
******************************************************************************/
TPage * VmLinearToVmPte( DWORD dwVmLin )
{
    static TPage static_pte = { TRUE };     /* Present */
    TPage *pEntry;
    DWORD dwPD, dwPTE;

    pEntry = &static_pte;

    if( PAGING_ENABLED )
    {
        dwPD = (dwVmLin >> 22) & 0x3FF;
        dwPTE = (dwVmLin >> 12) & 0x3FF;
        pEntry = &CPU.pPD[dwPD];

        if( pEntry->fPresent==TRUE )
        {
            ASSERT(pEntry->Index < Mem.MemVM.pages);
            pEntry = (TPage *) (Mem.MemVM.linear + 4096 * pEntry->Index );
            pEntry += dwPTE;
        }
    }
    else
        static_pte.Index = dwVmLin >> 12;

    return( pEntry );
}


/******************************************************************************
*                                                                             *
*  DWORD VmPageIndexToNtPageIndex( DWORD VmIndex )                            *
*                                                                             *
*******************************************************************************
*   Given a page number of a virtual page inside the VM, returns the NT's
*   effective index of that page within the host OS.  That index is suitable
*   to use within monitor PD and PTE pages.
*
*   TODO: Right now target page must be present, so the VM memory must be locked
*         but we may want to make it swappable in the future
*
*   Where:
*       VmIndex is the virtual page number (from VM page tables)
*
*   Returns:
*       Real effective page index as mapped by NT
******************************************************************************/
DWORD VmPageIndexToNtPageIndex( DWORD VmIndex )
{
    TPage *pPTE, *pHostPTE;
    DWORD hostLinear, dwPD, dwPTE;
    ASSERT(VmIndex < Mem.MemVM.pages );

    hostLinear = VmIndex * 4096 + Mem.MemVM.linear;
    dwPD = hostLinear >> 22;
    dwPTE = (hostLinear >> 12) & 0x3FF;

    pHostPTE = (TPage *) (NT_PAGE_DIR + dwPD * 4);
    ASSERT(pHostPTE->fPresent==TRUE);

    pPTE = (TPage *) (NT_PTE_BASE + (pHostPTE->Index << 12) + dwPTE * 4);
    ASSERT(pPTE->fPresent==TRUE);

    return( pPTE->Index );
}


/******************************************************************************
*                                                                             *
*  DWORD AnyNtVirtualToPhysical( DWORD dwAddress )                            *
*                                                                             *
*******************************************************************************
*   Given a virtual address (anywhere in the driver), returns the physical page
*   number.  The address have to be locked.
*
*   Where:
*       dwAddress is the driver address (locked/present)
*
*   Returns:
*       Real effective page index
******************************************************************************/
DWORD AnyNtVirtualToPhysical( DWORD dwAddress )
{
    TPage *pPTE, *pHostPTE;
    DWORD dwPD, dwPTE;

    dwPD = (dwAddress >> 22) & 0x3FF;
    dwPTE = (dwAddress >> 12) & 0x3FF;

    pHostPTE = (TPage *) (NT_PAGE_DIR + dwPD * 4);
    ASSERT(pHostPTE->fPresent==TRUE);

    pPTE = (TPage *) (NT_PTE_BASE + (pHostPTE->Index << 12) + dwPTE * 4);
    ASSERT(pPTE->fPresent==TRUE);

    return( pPTE->Index );
}

