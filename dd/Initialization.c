/******************************************************************************
*                                                                             *
*   Module:     Initialization.c                                              *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       3/14/2000                                                     *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the initialization code

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

/******************************************************************************
*                                                                             *
*   Global Variables                                                          *
*                                                                             *
******************************************************************************/

DWORD IntHandlers;

/******************************************************************************
*                                                                             *
*   Local Defines, Variables and Macros                                       *
*                                                                             *
******************************************************************************/

// Define if an exception pushes error code on the stack

int fHaveErrorCode [32] = {
    FALSE, FALSE, FALSE, FALSE,         // 0 - 3
    FALSE, FALSE, FALSE, FALSE,         // 4 - 7
    TRUE,  FALSE, TRUE,  TRUE,          // 8 - 11
    TRUE,  TRUE,  TRUE,  FALSE,         // 12 - 15

    FALSE, TRUE,  TRUE,  FALSE,         // 16 - 19
    FALSE, FALSE, FALSE, FALSE,
    FALSE, FALSE, FALSE, FALSE,
    FALSE, FALSE, FALSE, FALSE,
};

/******************************************************************************
*                                                                             *
*   Functions                                                                 *
*                                                                             *
******************************************************************************/

/******************************************************************************
*   DWORD GetGDTBase( TGDT_Gate *pGDT)                                        *
*******************************************************************************
*   Retrieves the base address stored in a GDT descriptor
*
*   Where:
*       pGDT is the address of a GDT descriptor
*
*   Returns:
*       base address field unscrambled
******************************************************************************/
DWORD GetGDTBase( TGDT_Gate *pGDT)
{
    return( pGDT->baseLow + (pGDT->baseMid<<16) + (pGDT->baseHigh<<24) );
}


/******************************************************************************
*   DWORD GetGDTLimit( TGDT_Gate *pGDT)                                       *
*******************************************************************************
*   Retrieves the limit field stored in a GDT descriptor
*
*   Where:
*       pGDT is the address of a GDT descriptor
*
*   Returns:
*       limit field unscrambled
******************************************************************************/
DWORD GetGDTLimit( TGDT_Gate *pGDT)
{
    return( (pGDT->limitLow + (pGDT->limitHigh<<16)) << (pGDT->granularity ? 12 : 0) );
}


/******************************************************************************
*   DWORD GetGDTBaseLimit( DWORD *pLimit, TGDT_Gate *pGDT)                    *
*******************************************************************************
*   Retrieves both the base and the limit field stored in a GDT descriptor
*
*   Where:
*       pLimit is the address of the variable to receive limit field
*       pGDT is the address of a GDT descriptor
*
*   Returns:
*       base address field unscrambled
******************************************************************************/
DWORD GetGDTBaseLimit( DWORD *pLimit, TGDT_Gate *pGDT)
{
    ASSERT(pLimit);
    *pLimit = (pGDT->limitLow + (pGDT->limitHigh<<16)) << (pGDT->granularity ? 12 : 0);
    return( pGDT->baseLow + (pGDT->baseMid<<16) + (pGDT->baseHigh<<24) );
}


/******************************************************************************
*                                                                             *
*   void SetIDT(TIDT_Gate *pIDT, DWORD selector, DWORD offset, int type)      *
*                                                                             *
*******************************************************************************
*
*   This function sets up an idt selector.  Selector is also marked present (P),
*   and marked as dpl=3 (user)
*
*   Where:
*       pIDT is a pointer to an IDT selector to set
*       selector is the selector field
*       offset is the offset field
*       type is the interrupt gate type
*
*   Returns:
*       void
*
******************************************************************************/
void SetIDT(TIDT_Gate *pIDT, DWORD selector, DWORD offset, int type)
{
    pIDT->offsetLow   = offset & 0xFFFF;
    pIDT->offsetHigh  = (offset >> 16) & 0xFFFF;
    pIDT->selector    = selector;
    pIDT->res         = 0;
    pIDT->type        = type;
    pIDT->dpl         = 3;
    pIDT->present     = TRUE;
}


/******************************************************************************
*                                                                             *
*   void SetGDT(TGDT_Gate *pGDT, DWORD base, DWORD limit, int type, int f32)  *
*                                                                             *
*******************************************************************************
*
*   This function sets up a gdt selector.  Selector is also marked present (P),
*   and the granularity is set to byte granular (if the limit is less or equal
*   to 0xFFFFF), or page granular.
*
*   We use this function to set up both Ring-0, 32-bit selectors for monitor
*   code/data/tss, and initial Ring-1, 16-bit selectors when guest enters
*   protected mode.
*
*   Where:
*       pGDT is a pointer to a GDT selector to set
*       base is the base address
*       limit is the limit expressed in bytes !
*       type is the selector type
*       f32 is a flag, set for Ring-0, 32-bit selector,
*                      reset for Ring-1, 16-bit selector
*   Returns:
*       void
*
******************************************************************************/
void SetGDT(TGDT_Gate *pGDT, DWORD base, DWORD limit, int type, BOOL f32)
{
    if( limit <= 0xFFFFF )
    {   // Make a selector byte granular
        pGDT->granularity = 0;              // Limit field is byte granular
    }
    else
    {   // Make a selector page granular
        limit >>= 12;
        pGDT->granularity = 1;              // Limit field is page granular
    }

    pGDT->baseLow     = base & 0xFFFF;
    pGDT->baseMid     = (base >> 16) & 0xFF;
    pGDT->baseHigh    = (base >> 24) & 0xFF;
    pGDT->limitLow    = limit & 0xFFFF;
    pGDT->limitHigh   = (limit >> 16) & 0xF;
    pGDT->type        = type;
    pGDT->dpl         = NOT( f32 );
    pGDT->present     = TRUE;
    pGDT->avail       = 0;
    pGDT->size32      = f32;
}


/******************************************************************************
*                                                                             *
*   void RebaseMonitor()                                                      *
*                                                                             *
*******************************************************************************
*
*   This function will rebase monitor data structures that are position
*   dependent.  It is called on initialization and after we decide to move
*   monitor to a new virtual address.  Since the linear address of the monitor
*   is constant in the meantime, this function injects it in the VM address
*   space as well.
*
*
*   Fields that are recalculated are:
*       pMonitor->guestGDT
*       pMonitor->guestIDT
*       pMonitor->offPD_Inject
*       pMonitor->fnTaskBridgeToVM
*       pMonitor->GDT for monitor CS
*       pMonitor->GDT for monitor DS
*       pMonitor->GDT for monitor TSS
*       pMonitor->VMPD[x] injects monitor pages into VM PD
*
******************************************************************************/
void RebaseMonitor()
{
    INFO(("Rebasing the monitor to %08X",pVM->dwVmMonitorLinear));

    // Monitor address *must* be 4-Mb aligned!

    ASSERT((pVM->dwVmMonitorLinear & ~0xFFC00000)==0);

    // Initialize gdt and idt descriptors to point to our monitor tables

    pMonitor->guestGDT.base  = pVM->dwVmMonitorLinear + offsetof(TMonitor, GDT);
    pMonitor->guestIDT.base  = pVM->dwVmMonitorLinear + offsetof(TMonitor, IDT);

    // Calculate the offset in the PD that we will overwrite for the task switch
    // This will make the monitor 4G page mapped in both host and guest

    pMonitor->offPD_Inject = pVM->dwVmMonitorLinear >> (22 - 2);

    // Set the address of the TaskBridgeToVM into the shared memory to jump to

    pMonitor->fn_TaskBridgeToVM = pVM->dwVmMonitorLinear + offsetof(TMonitor,BridgeCode);

    // Initialize Monitor's 32-bit Ring-0 CS and DS selectors in its address space

    SetGDT(&pMonitor->GDT[ VM_SEL_MONITOR_CS >> 3 ],
        pVM->dwVmMonitorLinear,
        sizeof(TMonitor), DESC_TYPE_EXEC, TRUE);

    SetGDT(&pMonitor->GDT[ VM_SEL_MONITOR_DS >> 3 ],
        pVM->dwVmMonitorLinear,
        sizeof(TMonitor), DESC_TYPE_DATA, TRUE);

    // Initialize task structure for a VM task

    SetGDT(&pMonitor->GDT[ VM_SEL_TSS >> 3 ],
        pVM->dwVmMonitorLinear + offsetof(TMonitor, guestTSS),
        sizeof(TSS_Struct) - 1, DESC_TYPE_TSS32A, TRUE);

    // Inject monitor PTE into a VM page directory

    pMonitor->VMPD[ pMonitor->offPD_Inject >> 2 ] = pMonitor->VMPDphysical;
}


/******************************************************************************
*                                                                             *
*   int InitVM()                                                              *
*                                                                             *
*******************************************************************************
*
*   This function initializes a virtual machine data structures and prepares
*   a VM to run.
*
*   Returns:
*       0 if successful
*       non-zero if it fails
*
******************************************************************************/
int InitVM()
{
    TGDT_Gate *pGDT;
    int i, gdt_index;
    DWORD dev;
    BYTE *pDest, *handler_address;

    TRACE(("Initializing VM..."));


    // Allocate all the memory needed for running a VM
    //========================================================================

    if( (i = AllocVMMemory()) != 0 )
        return( i );


    // Reset the state of the virtual CPU
    //========================================================================
    CPU_Reset();

    // Fill in CPUID information that will be given to a virtual machine

    if( !CpuHasCPUID() )
    {
        ASSERT(0);
        ERROR(("Can't run: Host CPU does not support CPUID instruction!?"));
        return( -1 );
    }

    GetCPUID(0, CPU.cpuid[0]);
    GetCPUID(1, CPU.cpuid[1]);
    GetCPUID(2, CPU.cpuid[2]);

    // TODO: Check if we run Intel??

    // Fake some weaker cpu

    CPU.cpuid[1][0] = 0x00000650;       // Family 6, model 5, stepping 0

    // Make sure to decrease the capabilities to what we can handle

    CPU.cpuid[1][3] = CPUID_FPU + CPUID_DE + CPUID_TSC + CPUID_MSR +
                      CPUID_CXS + CPUID_CMOV;

    // Initialize various devices; The order of initialization is important:
    //========================================================================
    RtlZeroMemory(&Device, sizeof(TDevice));
    InitMemoryAccess();                     // Initialize fault memory access
    InitSysReg();                           // Initialize system regions (gdt/idt/..)

    //========================================================================
    // Call device's registration function to give them a chance to register
    // their callbacks
    //========================================================================

    IoRegister          (&Device.Vdd[0]);    // Register IO faulting subsystem
    MemoryRegister      (&Device.Vdd[1]);    // Register Memory faulting subsystem
    PciRegister         (&Device.Vdd[2]);    // Register PCI fauting subsystem

    //========================================================================
    // CUSTOM VIRTUAL DEVICE REGISTRATION FUNCTIONS SHOULD BE PLACED HERE
    //========================================================================

    PIIX4_Register      (&Device.Vdd[3]);    // Register PIIX4 virtual chipset device
    CmosAndRtc_Register (&Device.Vdd[4]);    // Register CMOS RAM and RTC virtual devices
    Pic_Register        (&Device.Vdd[5]);    // Register virtual PIC device
    Pit_Register        (&Device.Vdd[6]);    // Register 8253/8254 PIT virtual chip device
    Dma_Register        (&Device.Vdd[7]);    // Register virtual DMA controller
    Keyboard_Register   (&Device.Vdd[8]);    // Register virtual keyboard chip device
    Vga_Register        (&Device.Vdd[9]);    // Register virtual VGA device
    Mda_Register        (&Device.Vdd[10]);   // Register virtual monochrome adapter device
    Floppy_Register     (&Device.Vdd[11]);   // Register virtual floppy device
    Hdd_Register        (&Device.Vdd[12]);   // Register virtual hard disk device(s)

    Device.VddTop = 13;

    //========================================================================
    // Call the initialization function of the each device in the order they
    // were registered
    //========================================================================

    for( dev=0; dev<Device.VddTop; dev++ )
    {
        if( Device.Vdd[dev].Initialize != NULL )
        {
            INFO(("Initializing virtual %s", Device.Vdd[dev].Name));
            (Device.Vdd[dev].Initialize)();
        }
    }


    //========================================================================
    // Rebase the monitor pages to the address we initially set
    //========================================================================
    // Set up some variables that are related to those rebased ones, but not
    // set by the function above

    pMonitor->guestGDT.limit = GDT_MAX * 8 - 1;
    pMonitor->guestIDT.limit = 256 * 8 - 1;

    CleanTPage(&pMonitor->VMPDphysical);
    pMonitor->VMPDphysical.Index    = Mem.MemMonitor.physical[ MONITOR_PTE_PTE ];
    pMonitor->VMPDphysical.fPresent = 1;
    pMonitor->VMPDphysical.fWrite   = 1;

    pMonitor->fn_NT_SEL_CS  = NT_SEL_CS;

    RebaseMonitor();

    // Initialize global CS selector that is used in the cross-task jump
    // to reload instruction TLB entry

    SetGDT(&pMonitor->GDT[ VM_SEL_GLOBAL_CS >> 3 ],
        0, 0xFFFFFFFF, DESC_TYPE_EXEC, TRUE);

    pMonitor->fn_VM_SEL_GLOBAL_CS = VM_SEL_GLOBAL_CS;

    //========================================================================
    // Copy TaskBridge code into the shared data section
    // We simply copy the position-independent code, and later build
    // interrupt handler functions
    //========================================================================
    // To make sure we dont overwrite our buffer, we store a check dword at
    // the end and later probe if it was still there
    *(DWORD*)(&pMonitor->BridgeCode[BRIDGE_CODE_SIZE]-4) = 0xCAFEBABE;
    //========================================================================

    // Find the size of the function code (Jump to VM) to be copied

    handler_address = pMonitor->BridgeCode;

    for( i=0; *(WORD *)((DWORD)TaskBridgeToVM+i)!=0xAA55; i++) ;

    // Copy the code into a buffer

    RtlCopyMemory(handler_address, TaskBridgeToVM, i);

    handler_address += i;

    // Find the size of the function code (Return stub from VM) to be copied

    for( i=0; *(WORD *)((DWORD)TaskBridgeToHost+i)!=0xAA55; i++) ;

    // Copy the code into a buffer

    RtlCopyMemory(handler_address, TaskBridgeToHost, i);

    pDest = handler_address + i;

    //================================================================================
    // Build interrupt handler stubs in the form of:
    //  Interrupt stubs for ints without error code on the ring-0 stack:
    //                           ------------------
    //  IntXX:  push    eax (dummy)                 0x50
    //          push    eax                         0x50
    //          mov     al, int#                    0xB0 Int#
    //          jmp     TaskBridgeToHost            0xE9 xx xx xx xx
    //
    //  Interrupt stubs for ints that place an error code on the ring-0 stack:
    //
    //  IntXX:
    //          push    eax                         0x50
    //          mov     al, int#                    0xB0 Int#
    //          jmp     TaskBridgeToHost            0xE9 xx xx xx xx
    //
    // and in the same time fill up IDT with those function addresses

    IntHandlers = (DWORD)pDest - (DWORD)pMonitor;

    for( i=0; i<256; i++)
    {
        // Set the IDT descriptor to address our interrupt stub with a 32 bit intr. gate

        SetIDT(&pMonitor->IDT[i], VM_SEL_MONITOR_CS, (DWORD)pDest - (DWORD)pMonitor, INT_TYPE_INT32);

        if( i<32 && fHaveErrorCode[i]==TRUE )
        {
            // Special CPU interrupts that push extra error code on the stack
        }
        else
        {
            // Interrupts that dont have extra error code on the stack

            *pDest++ = 0x50;
        }

        *pDest++ = 0x50;
        *pDest++ = 0xB0;
        *pDest++ = (BYTE) i;
        *pDest++ = 0xE9;

        *(DWORD *) pDest = handler_address - pDest - 4;

        pDest += 4;
    }

    //================================================================================
    // Check and make sure we did not overwrite code buffer

    if( *(DWORD*)(&pMonitor->BridgeCode[BRIDGE_CODE_SIZE]-4) != 0xCAFEBABE )
    {
        // Scream !!! and do something better than this...
        ASSERT("Bridge code too large to fit in a buffer!");
        return( -1 );
    }

    //========================================================================
    // Set up the rest of monitor structure
    //========================================================================

    pMonitor->hostCR3.Index  = Get_CR3_Index();
    pMonitor->guestCR3 = Mem.MemMonitor.physical[ MONITOR_PTE_PD ] << 12;
    pMonitor->guestCR4 = Get_CR4() &
        ~(BITMASK(CR4_PVI_BIT) | BITMASK(CR4_VME_BIT)) | BITMASK(CR4_TSD_BIT) | BITMASK(CR4_PCE_BIT);

    return( 0 );
}


/******************************************************************************
*                                                                             *
*   void ShutDownVM()                                                         *
*                                                                             *
*******************************************************************************
*
*   This function will shut down a VM; free all the data structures
*   associated with it.
*
******************************************************************************/
void ShutDownVM()
{
    int dev;
    TRACE(("Shutting down VM..."));

    // Since this function may be called multiple times (better be safe than
    // sorry :-) we need to guard against it

    if( pVM )
    {
        // Shut down all virtual devices
        ASSERT(Device.VddTop != 0);

        for( dev=Device.VddTop-1; dev>=0; dev--)
        {
            if( Device.Vdd[dev].Shutdown != NULL )
            {
                INFO(("Shutting down virtual %s", Device.Vdd[dev].Name));
                (Device.Vdd[dev].Shutdown)();
            }
#if DBG 
            memset(&Device.Vdd[dev], 0, sizeof(TVDD));
#endif
        }

        // Free all the outstanding memory blocks

        while( MemoryFree( 0xFFFFFFFF ) );

        // Invalidate major pointers, now they are pointing to freed memory, anyways

        pVM      = NULL;
        pMonitor = NULL;
    }
}
