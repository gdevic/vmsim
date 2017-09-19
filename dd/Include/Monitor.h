/******************************************************************************
*                                                                             *
*   Module:     Monitor.h													  *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       5/22/2000													  *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This is a header file containing monitor data structure that is
        a bridge between a device driver and a virtual machine

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 5/22/2000	 Original                                             Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Important Defines                                                         *
******************************************************************************/
#ifndef _MONITOR_H_
#define _MONITOR_H_

/******************************************************************************
*                                                                             *
*   Global Defines, Variables and Macros                                      *
*                                                                             *
******************************************************************************/

//-----------------------------------------------------------------------------
// This data structure defines the monitor that is mapped in the guest
// address space during the life of a guest, and in the host address space
// just briefly for transition into the guest.  It has to be 4Mb-aligned; to
// map it, simply store the physical address of its first page into a PD entry;
// the first 4K is a page table itself and will map the rest of it in 4Mb.
//-----------------------------------------------------------------------------
typedef struct
{
    // The monitor data is organized as follows:
    //     0x0000       1 page, page table entries to map itself
    //     0x1000       1 page, effective page directory when running a VM
    //     0x2000       2 pages, reserved
    //     0x4000       (MONITOR_SIZE_IN_PAGES-4) pages, monitor data

    TPage       MonitorPTE [ 1024 ];            // Used to map itself

#define MONITOR_PTE_PTE         0               // PT entry for monitor
#define MONITOR_PTE_PD          1               // PT entry for PD
#define MONITOR_PTE_PTE_CECP    2               // PT entry for PTE CECP'
#define MONITOR_PTE_CECP        3               // PT entry for CECP'

    TPage       VMPD [ 1024 ];                  // Effective VM page directory

    TPage       MonPTE_CECP [ 1024 ];           // 2 4K pages that are reserved
    BYTE        MonCECP     [ 4096 ];

    // -------------------------------------------------------------------------------
    // Cross-task monitor code and stack
    // -------------------------------------------------------------------------------

#define MONITOR_BRIDGE_OFFSET   (4096 * 4)

    BYTE        BridgeCode [BRIDGE_CODE_SIZE];  // Space for the bridge code
    BYTE        BridgeStack[BRIDGE_STACK_SIZE]; // Allocate some ring-0 stack space

    // -------------------------------------------------------------------------------
    // Array of pointers to the page table frames for each entry in the page directory
    // -------------------------------------------------------------------------------
    TPage      *pPTE[1024];                     // When paging is enabled

    // -------------------------------------------------------------------------------
    // Section describing the running guest; these are the actually used structures
    // when a VM is running:
    // -------------------------------------------------------------------------------

    TSS_Struct  guestTSS;                       // Effective TSS structure
    TGDT_Gate   GDT[GDT_MAX];                   // Effective GDT
    TIDT_Gate   IDT[256];                       // Effective IDT
    WORD        guestLDT;                       // Effective LDT
    TDescriptor guestGDT;                       // Cached global descriptor record
    TDescriptor guestIDT;                       // Cached interrupt descriptor record

    DWORD       guestCR2;                       // Effective PF address reg.
    DWORD       guestCR3;                       // Effective page directory reg.
    DWORD       guestCR4;                       // Effective CR4 register

    // -------------------------------------------------------------------------------
    // Section that keeps some host information for the duration of a guest run so
    // it can be restored every time the guest spins out:
    // -------------------------------------------------------------------------------

    TPage       hostCR3;                        // Temp host CR3 value
    TDescriptor hostGDT;                        // Temp host global descriptor value
    TDescriptor hostIDT;                        // Temp host interrupt descriptor value

    // -------------------------------------------------------------------------------
    // Miscellaneous for task switching
    // -------------------------------------------------------------------------------
    // offPD_Inject contains the offset from the page directory where to inject
    // VMPDphysical PT entry in order to map monitor.  The offset is common to both
    // host PD (at 0xC0300000) and guest PD (second page of the monitor structure).

    DWORD       offPD_Inject;                   // Where in the PD to inject...
    TPage       VMPDphysical;                   // ... and what PT entry to inject

    DWORD       fn_TaskBridgeToVM;              // Helper: address of the bridge-to-VM code
    DWORD       fn_NT_SEL_CS;                   // ... followed by CS for a far call

    DWORD       fn_CECP;                        // Helper: CECP in VM address space
    DWORD       fn_VM_SEL_GLOBAL_CS;            // ... followed by global CS selector

    DWORD       offPTE_CECP;                    // Offset of PTE entry to modify permissions

    DWORD       DriverESP;                      // Temp driver stack pointer

    int         nInterrupt;                     // VM current interrupt
    int         nErrorCode;                     // VM current error code
//    TPage      *pVM_PTE;                        // Last modified VM PTE entry (to restore)
    TPage       VM_PTE;                         // Original value of the VM PTE entry
    TMeta      *pMeta;                          // Address of the current meta data structure

} TMonitor;


#define MONITOR_SIZE_IN_PAGES       ((sizeof(TMonitor) >> 12) + 1 )


#endif //  _MONITOR_H_
