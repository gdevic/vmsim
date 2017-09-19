/******************************************************************************
*                                                                             *
*   Module:     MemoryAllocation.c                                            *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       5/22/2000                                                     *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the code for device driver memory allocations.

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 5/22/2000  Original                                             Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Include Files                                                             *
******************************************************************************/
#include <ntddk.h>                  // Include ddk function header file

#include "Vm.h"                     // Include root header file

#include "Queue.h"                  // Include queue management

#include "DeviceExtension.h"        // Include deviceExtension

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

// Define structure used to hold information about the memory that is being
// allocated and possibly shared with the user application process

typedef struct
{
    PVOID SystemVirtualAddress;
    PVOID UserVirtualAddress;
    PMDL  pMdl;
#if DBG
    DWORD dwSize;
#endif

} TMemEntry;


/******************************************************************************
*                                                                             *
*   Functions                                                                 *
*                                                                             *
******************************************************************************/

void MemoryAllocInit()
{
    PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)GUIDevice->DeviceExtension;

    QCreate(NonPagedPool, &pDevExt->qMemList);
}


/******************************************************************************
*                                                                             *
*   DWORD MemoryAlloc(DWORD dwSize, DWORD *pUser, BOOL fLocked)               *
*                                                                             *
*******************************************************************************
*
*   This function allocates a memory block from the kernel address space
*   and optionally maps the memory to the user process space as well.
*
*   The code keeps a linked list of blocks that were allocated so on the
*   driver exit, it frees them all.
*
*   Where:
*       dwSize is the size of the memory block
*       pUser, if not NULL, is the pointer of user address variable
*       fLocked, if TRUE, memory is allocated from the locked memory pool,
*               otherwise, it can be paged
*
*   Returns:
*       Address in the driver space of the memory block
*       NULL if allocation fails
*
******************************************************************************/
DWORD MemoryAlloc(DWORD dwSize, DWORD *pUser, BOOL fLocked)
{
    PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)GUIDevice->DeviceExtension;
    TMemEntry *pEntry;
    PMDL       pMdl;
    PVOID      SystemVirtualAddress;

    // If the memory is to be mapped to the user space, presume failure

    if( pUser ) *pUser = 0;

    // Allocate system memory

    SystemVirtualAddress = ExAllocatePoolWithTag( fLocked? NonPagedPool : PagedPool, dwSize, 'VSIM');

    if( SystemVirtualAddress )
    {
        // Allocate the structure for memory tracking

        pEntry = ExAllocatePoolWithTag( NonPagedPool, sizeof(TMemEntry), 'VSIM');

        if( pEntry )
        {
            // Prepare the mem track record to be inserted

            pEntry->SystemVirtualAddress = SystemVirtualAddress;
            pEntry->UserVirtualAddress   = 0;
            pEntry->pMdl                 = NULL;

            // Optionally, we may need to map the memory into user address space

            if( pUser )
            {
                pMdl = IoAllocateMdl( SystemVirtualAddress, dwSize, FALSE, FALSE, NULL);

                if( pMdl )
                {
                    // Build the MDL to describe the memory pages

                    MmBuildMdlForNonPagedPool(pMdl);

                    // Map the locked pages into process's user address space
#if 0
                    *pUser = (DWORD) MmMapLockedPages(pMdl, UserMode);
#else
                    *pUser = (DWORD) MmMapLockedPagesSpecifyCache(pMdl, UserMode, MmCached, NULL, FALSE, HighPagePriority );
#endif
                    pEntry->UserVirtualAddress   = (PVOID) *pUser;
                    pEntry->pMdl                 = pMdl;
                }

                if( (pMdl==NULL) || (*pUser==0) )
                {
                    ExFreePool(SystemVirtualAddress);
                    ExFreePool(pEntry);

                    ERROR(("Failed to allocate Mdl"));

                    return( 0 );
                }
            }
            // Insert the memory tracking record into a linked list

            QInsert(&pDevExt->qMemList, pEntry);
#if DBG
            // Set the newly allocated memory with a distinctive tag byte

            RtlFillMemory(SystemVirtualAddress, dwSize, 0x4D);
            pEntry->dwSize = dwSize;
#endif
        }
        else
        {
            ExFreePool(SystemVirtualAddress);
            SystemVirtualAddress = 0;

            ERROR(("Unable to allocate memory for MemEntry"));
        }
    }
    else
    {
        ERROR(("Unable to allocate %d bytes", dwSize));
    }

    return( (DWORD) SystemVirtualAddress );
}


//-----------------------------------------------------------------------------
// Helper function for MemoryFree() that compares memory addresses
//-----------------------------------------------------------------------------

static int cmpUserAddress( void *p1, const void *p2 )
{
    if( p1 )
    {
        if( p2 )
        {
            if( ((TMemEntry*)p1)->UserVirtualAddress == (PVOID)p2 )
                return(1);

            if( ((TMemEntry*)p1)->SystemVirtualAddress == (PVOID)p2 )
                return(1);

            if( (DWORD) p2 == 0xFFFFFFFF )   // Magic key always matches
                return(1);
        }
        else
        {
            ERROR(("cmpUserAddress: p2==NULL"));
        }
    }
    else
    {
        ERROR(("cmpUserAddress: p1==NULL"));
    }

    return(0);
}


/******************************************************************************
*                                                                             *
*   int MemoryFree( DWORD Address )                                           *
*                                                                             *
*******************************************************************************
*
*   This fucntion frees memory that was allocated using MemoryAlloc() call.
*   The address to free may be the system address or the user address if the
*   memory block was mapped into the user address space.
*
*   Where:
*       Address is the system or user address
*       0xFFFFFFFF is the magic key used when freeing all the memory.
*           It will free the last allocated memory block (from the linked list)
*
*   Returns:
*       Number of allocated blocks outstanding in the internal linked list
*
******************************************************************************/
int MemoryFree( DWORD Address )
{
    PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)GUIDevice->DeviceExtension;
    TMemEntry *pEntry;

    // We need to find this memory block that the user wants to free
    // within our memory tracking linked list

    if( QFind(&pDevExt->qMemList, cmpUserAddress, (const void *) Address) )
    {
        pEntry = (TMemEntry *) QCurrent(&pDevExt->qMemList);

        if( pEntry->UserVirtualAddress )
        {
            // Unmap locked pages from the user address space and free them
            // Well, we really can't do that since we dont know which context
            // we are running - it may not be the user process
            //
            // MmUnmapLockedPages(pEntry->UserVirtualAddress, pEntry->pMdl);

            // We can still free Mdl and memory buffer and clean up mem track list

            IoFreeMdl(pEntry->pMdl);
        }

        // Set the memory to be freed with a distinctive tag to reveal accidental use
#if DBG
        RtlFillMemory(pEntry->SystemVirtualAddress, pEntry->dwSize, 0x55);
#endif
        // Free the primary memory pool

        ExFreePool(pEntry->SystemVirtualAddress);

        // Delete a memory tracking node

        QDelete(&pDevExt->qMemList);

        // Free the memory tracking record

        ExFreePool(pEntry);
    }
    else
    {
        ERROR(("Trying to free memory that is not allocated!"));
    }

    return( pDevExt->qMemList.nSize );
}


/******************************************************************************
*                                                                             *
*   int MapPhysicalPages(TMemTrack *pMem, DWORD *physLookup)                  *
*                                                                             *
*******************************************************************************
*
*   Maps the address range into the physical page array and lookup array
*
*   Where:
*       pMem->linear is the linear address to map
*       pMem->pages is the number of pages to map
*       physlookup is the array of physical-to-virtual mapping
*
*   Returns:
*       1 if succeeds
*       0 if fails
*
******************************************************************************/
static int MapPhysicalPages(TMemTrack *pMem, DWORD *physLookup)
{
    PHYSICAL_ADDRESS physical;
    DWORD linearAddress;            // Virtual address for the loop
    int i;

    // Loop for each page in allocation and assign physical address

    for( i=0; i<(int)pMem->pages; i++ )
    {
        linearAddress = pMem->linear + i * 4096;

        // Get the physical address for that page...
        // Checks that the virtual address is valid

        ASSERT( MmIsAddressValid((PVOID)linearAddress) );

        // Store the page number into the OutputBuffer

        physical = MmGetPhysicalAddress((PVOID)linearAddress);

        pMem->physical[i] = physical.LowPart >> 12;

        // Set the physical page lookup

        Mem.physLookup[ physical.LowPart >> 12 ] = linearAddress;
    }

    return( 1 );
}


/******************************************************************************
*                                                                             *
*   int AllocMemTrackMemory(TMemTrack *pMem, DWORD *physLookup, BOOL fLocked) *
*                                                                             *
*******************************************************************************
*
*   Allocates an array of memory pages defined by TMemTrack structure and
*   corresponding physical driver memory.  It also sets corresponding entries
*   of physLookup array.
*
*   Where:
*       pMem->pages is the number of pages to allocate
*       physlookup is the array of physical-to-virtual mapping
*       fLocked, is set, uses locked memory
*
*   Returns:
*       1   succeeds
*       0   failed
*
*       pMem->physical contains a pointer to allocated memory
*           containing physical page numbers
*       pMem->linear points to the actual memory block
*
******************************************************************************/
static int AllocMemTrackMemory(TMemTrack *pMem, DWORD *physLookup, BOOL fLocked)
{
    ASSERT(pMem);
    ASSERT(physLookup);
    ASSERT(pMem->pages > 0);
    ASSERT(pMem->pages < (64*1024*1024)/4096);               // 64 Mb MAX

    pMem->linear  = 0;

    // Allocate memory for the array of physical pages

    pMem->physical = (DWORD *) MemoryAlloc( pMem->pages * sizeof(DWORD), NULL, TRUE );

    if( pMem->physical )
    {
        // Allocate requested memory from the driver

        pMem->linear = (DWORD) MemoryAlloc( pMem->pages * 4096, NULL, fLocked );

        if( pMem->linear != 0 )
        {
            if(MapPhysicalPages(pMem, physLookup))
            {
                return( 1 );
            }
            // Failed - free the driver allocated buffer

            MemoryFree(pMem->linear);

            pMem->linear = 0;
        }
        // Could not allocate driver memory.. Free what we got

        MemoryFree((DWORD) pMem->physical);

        pMem->physical = NULL;
    }

    return( 0 );
}


/******************************************************************************
*                                                                             *
*   void FreeMemTrackMemory(TMemTrack *pMem)                                  *
*                                                                             *
*******************************************************************************
*
*   Frees the driver memory and corresponding physical page array
*
*   Where:
*       pMem->pages is the number of pages that are described
*       pMem->physical is the address of memory array
*
******************************************************************************/
static void FreeMemTrackMemory(TMemTrack *pMem)
{
    // Free the device driver allocated memory; linear and physical page tables

    if( pMem->linear )
        MemoryFree((DWORD) pMem->linear);

    if( pMem->physical )
        MemoryFree((DWORD) pMem->physical);

    // Zero out the structure

    pMem->physical = NULL;
    pMem->linear   = 0;
    pMem->pages    = 0;
}


/******************************************************************************
*                                                                             *
*   void SetMemoryMapping()                                                   *
*                                                                             *
*******************************************************************************
*
*   This function sets the initial memory mapping within a Monitor data.
*   Memory should be already allocated.  This function sets up a subset of
*   monitor data structure; only constant mapping portion (PD/PTE/...).
*
*   Where:
*       pVM->dwVmMonitorLinear is the VM-linear address to map monitor
*
*       Monitor is mapped in the following way:
*           0x0000       1 page, page table entries to map itself
*           0x1000       1 page, effective page directory when running a VM
*           0x2000       2 pages, reserved
*           0x4000       (MONITOR_SIZE_IN_PAGES-4) pages, monitor data
*
******************************************************************************/
static void SetMemoryMapping()
{
    TPage *pPTE;
    int i;

    ASSERT(pVM);
    ASSERT(pVM->dwVmMonitorLinear);
    ASSERT(Mem.MemVM.linear);
    ASSERT(Mem.MemMonitor.linear);

    // Set the global pointer to the monitor structure

    pMonitor = (TMonitor *) Mem.MemMonitor.linear;

    // Reset all VM memory to FF

    memset((void*) Mem.MemVM.linear, 0xFF, Mem.MemVM.pages * 4096);

    // Clean monitor data structure to 0

    RtlZeroMemory((void*) pMonitor, sizeof(TMonitor));

    // =======================================================================
    // Map monitor pages inside monitor PTE page to address itself
    // =======================================================================

    for( i=0; i<(int)Mem.MemMonitor.pages; i++ )
    {
        pMonitor->MonitorPTE[ i ].Index    = Mem.MemMonitor.physical[ i ];
        pMonitor->MonitorPTE[ i ].fPresent = TRUE;
        pMonitor->MonitorPTE[ i ].fUser    = FALSE;
        pMonitor->MonitorPTE[ i ].fWrite   = TRUE;
    }

    // =======================================================================
    // Map VM page directory to span the whole VM PTE region of 4Mb
    // =======================================================================

    for( i=0; i<1024; i++ )
    {
        pMonitor->VMPD[ i ].Index    = Mem.MemPTE.physical[ i ];
        pMonitor->VMPD[ i ].fPresent = TRUE;
        pMonitor->VMPD[ i ].fUser    = TRUE;
        pMonitor->VMPD[ i ].fWrite   = TRUE;
    }

    // =======================================================================
    // Map base user memory from VM linear 0 inside VM PTE tables
    // =======================================================================

    pPTE = (TPage*) Mem.MemPTE.linear;
    Mem.pPagePTE = (TPage *) pPTE;

    RtlZeroMemory((void*) pPTE, 1024 * 1024 * sizeof(TPage));       // Clean PTE

    for( i=0; i < (int)Mem.MemVM.pages; i++ )
    {
        pPTE[ i ].Index    = Mem.MemVM.physical[i];
        pPTE[ i ].fPresent = TRUE;
        pPTE[ i ].fUser    = TRUE;
        pPTE[ i ].fWrite   = TRUE;
    }
}


/******************************************************************************
*                                                                             *
*   int AllocVMMemory()                                                       *
*                                                                             *
*******************************************************************************
*
*   This function allocates VM memory and initializes those structures inside
*   the VM
*
*   Where:
*       (global) pVM is the address of a VM structure where:
*           ->dwVmMemorySize specifies the requested guest VM memory size
*           ->dwHostMemorySize specifies the amount of memory on the host
*
*   Returns:
*       0  for succeess
*       non-zero for failure
*
******************************************************************************/
int AllocVMMemory()
{
    DWORD physLookupSize, metaLookupSize, stampLookupSize;
    int i;

    INFO(("VM physical memory size set to: %d Mb", pVM->dwVmMemorySize/(1024*1024) ));
    INFO(("Amount of host physical memory: %d Mb", pVM->dwHostMemorySize/(1024*1024) ));

    ASSERT(pVM->dwVmMemorySize);
    ASSERT(pVM->dwHostMemorySize);

    // Set the required number of pages to allocate in each category

    Mem.MemVM.pages = pVM->dwVmMemorySize >> 12;

    Mem.MemPTE.pages = (1024*1024*4) / 4096;            // 4Mb for PTE

    Mem.MemMonitor.pages = MONITOR_SIZE_IN_PAGES;       // Monitor pages

    Mem.MemAccess.pages = 2;                            // 2 spare pages for mem access

    // Allocate physLookup table that maps physical page numbers into
    // the driver allocated virtual addresses for allocated memory regions

    physLookupSize = (pVM->dwHostMemorySize / 4096) * sizeof(void *);
    Mem.physLookup = (DWORD *) MemoryAlloc( physLookupSize, NULL, FALSE /* Not locked ! */ );

    // Allocate metaLookup table that maps VM physical page indices into driver pointers
    // to meta structure for all cached code pages

    metaLookupSize = Mem.MemVM.pages * sizeof(TMeta *);
    Mem.metaLookup = (TMeta **) MemoryAlloc( metaLookupSize, NULL, TRUE /* Locked ! */ );

    // Allocate pPageStamp byte-array that keeps the "stamps" of each VM
    // physical page on what it contains for being stamped Read-Only

    stampLookupSize = Mem.MemVM.pages * sizeof(BYTE *);
    Mem.pPageStamp = (BYTE *) MemoryAlloc( stampLookupSize, NULL, FALSE /* Not locked ! */ );

    if( Mem.physLookup && Mem.metaLookup && Mem.pPageStamp )
    {
        // Clean physical lookup arrays and stamps

        RtlZeroMemory((void*) Mem.physLookup, physLookupSize );
        RtlZeroMemory((void*) Mem.metaLookup, metaLookupSize );
        RtlZeroMemory((void*) Mem.pPageStamp, stampLookupSize );

        // Allocate and map memory for VM itself, VM page table, monitor memory regions
        // and memory access spare pages

        if( AllocMemTrackMemory( &Mem.MemVM, Mem.physLookup, TRUE ))
        {
            if( AllocMemTrackMemory( &Mem.MemPTE, Mem.physLookup, TRUE ))
            {
                if( AllocMemTrackMemory( &Mem.MemMonitor, Mem.physLookup, TRUE ))
                {
                    if( AllocMemTrackMemory( &Mem.MemAccess, Mem.physLookup, TRUE ))
                    {
                        // Allocate meta data pages

                        for( i=0; i<MAX_META_DATA; i++ )
                        {
                            Mem.Meta[i].MemPage.pages = 2;

                            if( ! AllocMemTrackMemory( &Mem.Meta[i].MemPage, Mem.physLookup, FALSE /* Not locked ! */ ))
                                break;

                            CleanTPage(&Mem.Meta[i].VmCodePage);
                        }

                        if( i==MAX_META_DATA )
                        {
                            // Initialize meta data structures

                            MetaInitialize();

                            // Set the monitor memory mappings

                            SetMemoryMapping();

                            return( 0 );
                        }

                        // Ooops! Allocation failed.

                        for( ; i>0; i-- )
                            FreeMemTrackMemory(&Mem.Meta[i-1].MemPage);

                        FreeMemTrackMemory(&Mem.MemAccess);
                    }
                    FreeMemTrackMemory(&Mem.MemMonitor);
                }
                FreeMemTrackMemory(&Mem.MemPTE);
            }
            FreeMemTrackMemory(&Mem.MemVM);
        }
        MemoryFree((DWORD) Mem.physLookup);
        MemoryFree((DWORD) Mem.metaLookup);
        MemoryFree((DWORD) Mem.pPageStamp);
    }

    Mem.physLookup = NULL;
    Mem.metaLookup = NULL;
    Mem.pPageStamp = NULL;

    return( -1 );
}



/******************************************************************************
*                                                                             *
*   void FreeVMMemory()                                                       *
*                                                                             *
*******************************************************************************
*
*   This function frees VM memory resources and clears the structures inside
*   the given VM
*
******************************************************************************/
void FreeVMMemory()
{
    int i;

    // Free all device allocated memory and arrays

    for( i=0; i<MAX_META_DATA; i++ )
        FreeMemTrackMemory(&Mem.Meta[i].MemPage);

    FreeMemTrackMemory(&Mem.MemAccess);
    FreeMemTrackMemory(&Mem.MemVM);
    FreeMemTrackMemory(&Mem.MemPTE);
    FreeMemTrackMemory(&Mem.MemMonitor);

    MemoryFree((DWORD) Mem.physLookup);
    MemoryFree((DWORD) Mem.metaLookup);
    MemoryFree((DWORD) Mem.pPageStamp);

    Mem.physLookup = NULL;
    Mem.metaLookup = NULL;
    Mem.pPageStamp = NULL;
}


/******************************************************************************
*                                                                             *
*   int MemoryCheck()                                                         *
*                                                                             *
*******************************************************************************
*
*   This function simply checks the integrity of the memory queue structures
*
*   Returns:
*       QueueCheck return code
*
******************************************************************************/
int MemoryCheck()
{
    PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)GUIDevice->DeviceExtension;

    return( QCheck(&pDevExt->qMemList) );
}


/******************************************************************************
*                                                                             *
*   void MemoryMapROM( PVOID pSrc, DWORD dwSize, DWORD dwVmPhysical)          *
*                                                                             *
*******************************************************************************
*
*   Maps the image of a ROM code inside the virtual machine address space.
*
*   Where:
*       pSrc is the source address of the image to map
*       dwSize is the size of the image in bytes
*       dwVmPhysical is the physical address inside the VM to map to
*
******************************************************************************/
void MemoryMapROM( PVOID pSrc, DWORD dwSize, DWORD dwVmPhysical)
{
    int i, romRegion, romSize;
    TPage *pPTE;
    ASSERT(Mem.MemVM.linear);

    TRACE(("Mapping ROM image to %08X (%dK)", dwVmPhysical, dwSize/4096));

    // Copy the resource into the VM address space:
    // We simulate shadow ROM by simply copying ROM image to the VM physical
    // memory.
    // TODO: Shadow ROM - This approach will not work when paging is on...

    memcpy((void*)(Mem.MemVM.linear + dwVmPhysical), pSrc, dwSize);

#if 0
#if DBG
//********************************************************************************
// TESTING AREA 51
//
// Stuck your little test code stubs here and they will be copied at the address
// 0 and executed on VM startup.
//
//********************************************************************************
// This is to ease debugging: A custom ROM code may be embedded at the beginning
// of the physical VM memory.  If we are mapping anything at the address 0,
// we know it is a test code, so we adjust cs:eip to 0:0

    if( dwVmPhysical==0 )
    {
        INFO(("Loading test code at 0000:0000, resetting CS:EIP"));

        pMonitor->guestTSS.eip = 0x0000;
        pMonitor->guestTSS.cs  = 0x0000;

        {
            BYTE *p = (BYTE *) Mem.MemVM.linear;

            *p++ = 0xEB;
            *p++ = 0xFE;


#if 0       // Testing memory access to the same code page
            *p++ = 0xA1;        // mov ax, [0]
            *p++ = 0x00;
            *p++ = 0x00;
            *p++ = 0x40;        // inc ax
            *p++ = 0xA3;        // mov [0], ax
            *p++ = 0x00;
            *p++ = 0x00;
#endif
#if 0       // Testing memory access to the registered faulting region
            *p++ = 0xB8;        // mov ax, b000
            *p++ = 0x00;
            *p++ = 0xB0;
            *p++ = 0x8E;        // mov ds, ax
            *p++ = 0xD8;
            *p++ = 0xA1;        // mov ax, [0]
            *p++ = 0x00;
            *p++ = 0x00;
            *p++ = 0x40;        // inc ax
            *p++ = 0xA3;        // mov [0], ax
            *p++ = 0x00;
            *p++ = 0x00;
            *p++ = 0x40;        // inc ax
            *p++ = 0x87;        // xchg ax, [0]
            *p++ = 0x06;
            *p++ = 0x00;
            *p++ = 0x00;
            *p++ = 0x90;        // nop
#endif
        }
    }

//********************************************************************************
#endif // DBG
#endif
}
