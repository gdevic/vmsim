/******************************************************************************
*                                                                             *
*   Module:     CPU.h                                                         *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       5/22/2000                                                     *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This is a header file containing the virtual CPU data structures

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 5/22/2000  Original                                             Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Important Defines                                                         *
******************************************************************************/
#ifndef _CPU_H_
#define _CPU_H_

/******************************************************************************
*                                                                             *
*   Global Defines, Variables and Macros                                      *
*                                                                             *
******************************************************************************/

#define PAGING_ENABLED      (CPU.cr0 & BITMASK(CR0_PG_BIT))
#define PERCIEVED_CPL       ((pMonitor->guestTSS.cs & 2)==0? 0 : 3)
#define VM_CPL              ((DWORD) pMonitor->guestTSS.cs & 3)

//-----------------------------------------------------------------------------
// We keep a linked list of registered system ranges (GDT,IDT,TSS,..) and
// do callback on page fault
//-----------------------------------------------------------------------------
enum eSysReg { SYSREG_TSS = 0, SYSREG_GDT, SYSREG_LDT, SYSREG_IDT, SYSREG_PD, SYSREG_PTE, SYSREG_MAX };

//-----------------------------------------------------------------------------
// Describe the Virtual Machine constants
//-----------------------------------------------------------------------------

// These are our entries in the GDT, starting at the top of the GDT

#define VM_SEL_MONITOR_CS      ((GDT_MAX-1 )*8)     // Monitor code selector
#define VM_SEL_MONITOR_DS      ((GDT_MAX-2 )*8)     // Monitor data selector
#define VM_SEL_TSS             ((GDT_MAX-3 )*8)     // TSS selector of a VM task
#define VM_SEL_GLOBAL_CS       ((GDT_MAX-4 )*8)     // Global CS sel. for task switch
#define VM_SEL_REAL_CS         ((GDT_MAX-5 )*8 + SEL_RPL1_MASK)     // `Real' mode CS selector
#define VM_SEL_REAL_DS         ((GDT_MAX-6 )*8 + SEL_RPL1_MASK)     // `Real' mode DS selector
#define VM_SEL_REAL_ES         ((GDT_MAX-7 )*8 + SEL_RPL1_MASK)     // `Real' mode ES selector
#define VM_SEL_REAL_FS         ((GDT_MAX-8 )*8 + SEL_RPL1_MASK)     // `Real' mode FS selector
#define VM_SEL_REAL_GS         ((GDT_MAX-9 )*8 + SEL_RPL1_MASK)     // `Real' mode GS selector
#define VM_SEL_REAL_SS         ((GDT_MAX-10)*8 + SEL_RPL1_MASK)     // `Real' mode SS selector
#define GDT_SEL_VM_TOP         ((GDT_MAX-10)*8)     // VM can not reach this value

// These dont change on NT 5 so we can use them as constants

#define NT_SEL_CS       0x0008
#define NT_SEL_DS       0x0023
#define NT_SEL_SS       0x0010
#define NT_SEL_ES       0x0023
#define NT_SEL_FS       0x0030
#define NT_SEL_GS       0x0000
#define NT_SEL_TSS      0x0028
#define NT_PAGE_DIR     0xC0300000
#define NT_PTE_BASE     0x80000000


typedef struct
{
    int CPU_mode;               // Which mode the virtual CPU is running:

    BOOL fSelPMode[6];          // *S registers are selectors, not segments

    BOOL fResetPending;         // CPU needs to be reset
    BOOL fInterrupt;            // PIC asked for an (external) interrupt
    BOOL fCurrentMetaInvalidate;// Request to invalidate current meta page
    BOOL fMetaInvalidate;       // Request to invalidate all meta pages

    TSS_Struct *pTSS;           // Pointer to VM TSS table in driver space
    TGDT_Gate *pGDT;            // Pointer to VM GDT table in driver space
    TIDT_Gate *pIDT;            // Pointer to VM IDT table in driver space
    TPage *pPD;                 // Pointer to VM Page Directory in driver space
    TPage *pPTE[1024];          // Array of pointers to VM PTE, shadows VM PD

    //=====================================================================
    // Here we store values of system CPU registers that the virtual
    // machine thinks it is using.
    //=====================================================================

    TDescriptor guestGDT;       // Guest percieved global descriptor value
    TDescriptor guestIDT;       // Guest percieved interrupt descriptor value

    DWORD cr0;                  // Guest percieved CR0 register
    DWORD cr2;                  // Guest percieved CR2 register
    DWORD cr3;                  // Guest percieved CR3 register
    DWORD cr4;                  // Guest percieved CR4 register

    DWORD dr0;                  // Guest percieved DR0 register
    DWORD dr1;                  // Guest percieved DR1 register
    DWORD dr2;                  // Guest percieved DR2 register
    DWORD dr3;                  // Guest percieved DR3 register
    DWORD dr6;                  // Guest percieved DR6 register
    DWORD dr7;                  // Guest percieved DR7 register

    WORD  guestTR;              // Guest percieved Task Register
    DWORD cpuid[3][4];          // CPUID response on [eax=0..2][eax|ebx|ecx|edx]

    DWORD eflags;               // Virtual part of VM's flags: VM, IF

///////////////////////////////////////////////////////////////////////////////
//    31 -  21 20  19  18 17 16  15 14 1312 11 10 9  8  7  6  5 4  3 2  1 0
//    -----------------------------------------------------------------------
//    0 - 0 ID VIP VIF AC VM RF | 0 NT IOPL OF DF IF TF SF ZF 0 AF 0 PF 1 CF
//    -----------------------------------------------------------------------
// active   X                   |      XX   X  X        X  X  0 X  0 X  1 X
// eflags                 X     |   X             X  X
//
// 'active' bits are kept in the current pMonitor->guestTSS.eflags
// 'eflags' bits are virtualized.  What VM thinks it has is stored in vCPU.eflags
// and presented with virtual instructions pushf, interrupts etc., eg. faked.
//
// TODO: Handle VM's TF and RF
//
#define EFLAGS_ACTIVE_MASK  0x00203CFF  // Pass bits that can be run inside the VM
//
// Takes care that all constant bits of 0 and 1 are in place
//
#define EFLAGS_SET_CONSTANTS(eflags)    (((eflags) & 0x003F7FD5) | 2)
//
///////////////////////////////////////////////////////////////////////////////

} TCPU;


#endif //  _CPU_H_
