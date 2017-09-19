/******************************************************************************
*                                                                             *
*   Module:     CPU.c                                                         *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       3/14/2000                                                     *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        Virtual CPU code

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
#include "ntddk.h"                  // Include driver C level functions

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

#define CPU_FAMILY_ID       6       // 6 - Pentium Pro
#define CPU_MODEL_ID        5       // Model number beginning with 0001b
#define CPU_STEPPING_ID     0       // Stepping number

/******************************************************************************
*                                                                             *
*   Functions                                                                 *
*                                                                             *
******************************************************************************/

/******************************************************************************
*                                                                             *
*   void CPU_Reset(void)                                                      *
*                                                                             *
*******************************************************************************
*
*   This function will reset the virtual CPU.  The state will be set to
*   continue running at the initial CPU address (F000:FFF0) in real mode.
*
******************************************************************************/
void CPU_Reset(void)
{
    TSS_Struct *pGuestTSS;

    TRACE(("CPU_Reset()"));
    ASSERT(pVM);
    ASSERT(pMonitor);

    //========================================================================
    // Clear the CPU structure and set it to run virtual "real" mode
    //========================================================================

    RtlZeroMemory(&CPU, sizeof(TCPU));

    CPU.fResetPending  = 0;
    CPU.CPU_mode       = CPU_REAL_MODE;
    CPU.eflags         = EFLAGS_SET_CONSTANTS(0);
    CPU.guestIDT.base  = 0x00000000;
    CPU.guestIDT.limit = 0x03FF;

    //========================================================================
    // Initialize VM task structure for the initial jump into the VM code
    //========================================================================

    pGuestTSS = &pMonitor->guestTSS;

    pGuestTSS->link   = 0;
    pGuestTSS->esp0   = offsetof(TMonitor,BridgeStack) + BRIDGE_STACK_SIZE;
    pGuestTSS->ss0    = VM_SEL_MONITOR_DS;
    pGuestTSS->eip    = 0x0000FFF0;
    pGuestTSS->eflags = EFLAGS_SET_CONSTANTS( VM_MASK + IF_MASK );
    pGuestTSS->eax    = 0;
    pGuestTSS->ecx    = 0;
    pGuestTSS->edx    = (CPU_FAMILY_ID << 8) | (CPU_MODEL_ID << 4) | CPU_STEPPING_ID;
    pGuestTSS->ebx    = 0;
    pGuestTSS->esp    = 0;
    pGuestTSS->ebp    = 0;
    pGuestTSS->esi    = 0;
    pGuestTSS->edi    = 0;
    pGuestTSS->es     = 0;
    pGuestTSS->cs     = 0xF000;
    pGuestTSS->ss     = 0;
    pGuestTSS->ds     = 0;
    pGuestTSS->fs     = 0;
    pGuestTSS->gs     = 0;
    pGuestTSS->ldt    = 0;
    pGuestTSS->debug  = 0;
    pGuestTSS->ioperm = offsetof(TSS_Struct, ioperm_map);

    // Set IO permission bitmap to trap all IO accesses

    memset(pMonitor->guestTSS.ioperm_map, 0xFF, MAX_IOPERM);

    // Reset virtual system registers

    CPU.cr0 = 0x60000010;
    CPU.cr2 = 0;
    CPU.cr3 = 0;
    CPU.cr4 = 0;

    CPU.dr0 = 0;
    CPU.dr1 = 0;
    CPU.dr2 = 0;
    CPU.dr3 = 0;
    CPU.dr6 = 0xFFFF0FF0;
    CPU.dr7 = 0x00000400;

    // Fill in CPUID information that will be given to a virtual machine

    GetCPUID(0, CPU.cpuid[0]);
    GetCPUID(1, CPU.cpuid[1]);
    GetCPUID(2, CPU.cpuid[2]);

    CPU.cpuid[1][0] = (CPU_FAMILY_ID << 8) | (CPU_MODEL_ID << 4) | CPU_STEPPING_ID;
    CPU.cpuid[1][3] &=
        CPUID_FPU + CPUID_DE + CPUID_TSC + CPUID_MSR + CPUID_CXS + CPUID_CMOV;
}


/******************************************************************************
*                                                                             *
*   void SetCpuMode( DWORD CpuMode )                                          *
*                                                                             *
*******************************************************************************
*
*   Tracks the execution mode of a running VM.
*
******************************************************************************/
void SetCpuMode( DWORD CpuMode )
{
    CPU.CPU_mode = CpuMode;

    switch( CpuMode )
    {
        case CPU_REAL_MODE:
        case CPU_V86_MODE:
                pMonitor->guestTSS.eflags |= VM_MASK;
            break;

        case CPU_KERNEL_MODE:
        case CPU_USER_MODE:
                pMonitor->guestTSS.eflags &= ~VM_MASK;
            break;

        default:  ASSERT(0);
    }
}


// ---------------------------------------------------------------------------
// For pmode, assume stack selector is 16-bit
// ---------------------------------------------------------------------------
WORD CPU_PopWord(void)
{
    DWORD dwHostLinear;
    DWORD dwVmLin;
    WORD value;

    // TODO: Make sure the stack is on the even address boundary (AC flag?)
    switch( CPU.CPU_mode )
    {
        case CPU_REAL_MODE:
            dwVmLin = (pMonitor->guestTSS.ss << 4) + LOWORD(pMonitor->guestTSS.esp);
            break;

        case CPU_V86_MODE:
        case CPU_KERNEL_MODE:
            dwVmLin = GetGDTBase(&pMonitor->GDT[ pMonitor->guestTSS.ss >> 3 ]) + LOWORD(pMonitor->guestTSS.esp);
            break;
    }

    if(dwHostLinear = VmLinearToDriverLinear(dwVmLin))
    {
        value = *(WORD *)dwHostLinear;
        *(WORD *)&pMonitor->guestTSS.esp = LOWORD(pMonitor->guestTSS.esp + 2);
    }
    else
    {
        // Stack page is not present
        ASSERT(0);
    }

    return( value );
}

// ---------------------------------------------------------------------------
// For pmode, assume stack selector is 32-bit
// ---------------------------------------------------------------------------
DWORD CPU_PopDword(void)
{
    DWORD dwHostLinear;
    DWORD dwVmLin;
    DWORD value;

    // TODO: Make sure the stack is on the even address boundary (AC flag?)
    switch( CPU.CPU_mode )
    {
        case CPU_REAL_MODE:
            dwVmLin = (pMonitor->guestTSS.ss << 4) + LOWORD(pMonitor->guestTSS.esp);
            break;

        case CPU_V86_MODE:
        case CPU_KERNEL_MODE:
            dwVmLin = GetGDTBase(&pMonitor->GDT[ pMonitor->guestTSS.ss >> 3 ]) + pMonitor->guestTSS.esp;
            break;
    }

    if(dwHostLinear = VmLinearToDriverLinear(dwVmLin ))
    {
        value = *(DWORD *)dwHostLinear;
        pMonitor->guestTSS.esp += 4;
    }
    else
    {
        // Stack page is not present
        ASSERT(0);
    }

    return( value );
}

// ---------------------------------------------------------------------------
// For pmode, assume stack selector is 16-bit
// ---------------------------------------------------------------------------
int CPU_PushWord(WORD wWord)
{
    DWORD dwHostLinear;
    DWORD dwVmLin;

    // TODO: Make sure the stack is on the even address boundary (AC flag?)
    switch( CPU.CPU_mode )
    {
        case CPU_REAL_MODE:
            dwVmLin = (pMonitor->guestTSS.ss << 4) + LOWORD(pMonitor->guestTSS.esp - 2);
            break;

        case CPU_V86_MODE:
        case CPU_KERNEL_MODE:
            dwVmLin = GetGDTBase(&pMonitor->GDT[ pMonitor->guestTSS.ss >> 3 ]) + LOWORD(pMonitor->guestTSS.esp - 2);
            break;
    }

    if(dwHostLinear = VmLinearToDriverLinear(dwVmLin))
    {
        *(WORD *)dwHostLinear = wWord;
        *(WORD *)&pMonitor->guestTSS.esp = LOWORD(pMonitor->guestTSS.esp - 2);
    }
    else
    {
        // Stack page is not present
        ASSERT(0);
    }

    return( 0 );
}

// ---------------------------------------------------------------------------
// For pmode, assume stack selector is 32-bit
// ---------------------------------------------------------------------------
int CPU_PushDword(DWORD dwDword)
{
    DWORD dwHostLinear;
    DWORD dwVmLin;

    // TODO: Make sure the stack is on the even address boundary (AC flag?)
    switch( CPU.CPU_mode )
    {
        case CPU_REAL_MODE:
            dwVmLin = (pMonitor->guestTSS.ss << 4) + LOWORD(pMonitor->guestTSS.esp - 4);
            break;

        case CPU_V86_MODE:
        case CPU_KERNEL_MODE:
            dwVmLin = GetGDTBase(&pMonitor->GDT[ pMonitor->guestTSS.ss >> 3 ]) + pMonitor->guestTSS.esp - 4;
            break;
    }

    if(dwHostLinear = VmLinearToDriverLinear(dwVmLin))
    {
        *(DWORD *)dwHostLinear = dwDword;
        pMonitor->guestTSS.esp -= 4;
    }
    else
    {
        // Stack page is not present
        ASSERT(0);
    }

    return( 0 );
}


/******************************************************************************
*      int CPU_Interrupt( BYTE bIntNumber )                                   *
*******************************************************************************
*   This function is used to simulate CPU interrupts, both asynchronous
*   and programmatic (INT n instruction).
*
*   Where:
*       bIntNumber is the interrupt number to set up
*   Returns:
*       0
******************************************************************************/
int CPU_Interrupt( BYTE bIntNumber )
{
    DWORD dwVector, oldSS, oldESP;

    switch( CPU.CPU_mode )
    {
        case CPU_REAL_MODE:
                // Store the current state on the VM stack
                CPU_PushWord( LOWORD((pMonitor->guestTSS.eflags & EFLAGS_ACTIVE_MASK)
                                   | (CPU.eflags & ~EFLAGS_ACTIVE_MASK)) );
                CPU_PushWord( pMonitor->guestTSS.cs );
                CPU_PushWord( LOWORD(pMonitor->guestTSS.eip) );
                // (Real mode does not store error code)
                // Interrupt affects the following flags (clears)
                // TODO: Something's fishy here - do we update monitor working copy with this?
                CPU.eflags &= ~(IF_MASK | TF_MASK | AC_MASK);

                // Get the interrupt vector from the IVT at base address 0
                dwVector = *(DWORD *)(Mem.MemVM.linear + bIntNumber * 4);

                // Store the new execution address into cs:ip
                pMonitor->guestTSS.cs = HIWORD(dwVector);
                pMonitor->guestTSS.eip = LOWORD(dwVector);
            break;

        case CPU_V86_MODE:
            // VM is in protected mode and its monitor was executing a user code in V86
            // mode.  It has set up his own TSS, from which we take stack.
            ASSERT(0);
                // Copy the V86 stack frame to the virtual machine supervisor
                // stack as set by the guest TSS
                oldSS  = pMonitor->guestTSS.ss;
                oldESP = pMonitor->guestTSS.esp;

                pMonitor->guestTSS.ss  = CPU.pTSS->ss0 | SEL_RPL1_MASK;
                pMonitor->guestTSS.esp = CPU.pTSS->esp0;

                CPU_PushDword( pMonitor->guestTSS.gs );
                CPU_PushDword( pMonitor->guestTSS.fs );
                CPU_PushDword( pMonitor->guestTSS.ds );
                CPU_PushDword( pMonitor->guestTSS.es );
                CPU_PushDword( oldSS );
                CPU_PushDword( oldESP );
                CPU_PushDword( pMonitor->guestTSS.eflags );
                CPU_PushDword( pMonitor->guestTSS.cs );
                CPU_PushDword( pMonitor->guestTSS.eip );

                // Store the new execution address into cs:ip
                pMonitor->guestTSS.cs = (WORD) CPU.pIDT[bIntNumber].selector | SEL_RPL1_MASK;
                pMonitor->guestTSS.eip = (CPU.pIDT[bIntNumber].offsetHigh << 16) + CPU.pIDT[bIntNumber].offsetLow;
            break;

        case CPU_KERNEL_MODE:
            ASSERT(0);
            if( pMonitor->GDT[ pMonitor->guestTSS.ss >> 3 ].size32 )
            {
                // 32 bit stack selector
                // Store the current state on the VM stack
                CPU_PushDword( (pMonitor->guestTSS.eflags & EFLAGS_ACTIVE_MASK)
                                   | (CPU.eflags & ~EFLAGS_ACTIVE_MASK) );
                CPU_PushDword( (DWORD) pMonitor->guestTSS.cs );
                CPU_PushDword( pMonitor->guestTSS.eip );

                // Store the new execution address into cs:ip
                pMonitor->guestTSS.cs = (WORD) CPU.pIDT[bIntNumber].selector | SEL_RPL1_MASK;
                pMonitor->guestTSS.eip = (CPU.pIDT[bIntNumber].offsetHigh << 16) + CPU.pIDT[bIntNumber].offsetLow;
            }
            else
            {
                // 16 bit stack selector
                // Store the current state on the VM stack
                CPU_PushWord( LOWORD((pMonitor->guestTSS.eflags & EFLAGS_ACTIVE_MASK)
                                   | (CPU.eflags & ~EFLAGS_ACTIVE_MASK)) );
                CPU_PushWord( pMonitor->guestTSS.cs );
                CPU_PushWord( LOWORD(pMonitor->guestTSS.eip) );

                // Store the new execution address into cs:ip
                pMonitor->guestTSS.cs = (WORD) CPU.pIDT[bIntNumber].selector | SEL_RPL1_MASK;
                pMonitor->guestTSS.eip = (CPU.pIDT[bIntNumber].offsetHigh << 16) + CPU.pIDT[bIntNumber].offsetLow;
            }
            break;
    }
    return( 0 );
}


/******************************************************************************
*      int CPU_Exception( BYTE bExceptionNumber, DWORD ErrorCode )            *
*******************************************************************************
*   This function is used to simulate CPU exceptions.  For the purpose of
*   keeping definitions clean, we define exceptions all internal CPU faults,
*   traps and aborts, but not software interrupts INT n.
*
*   Where:
*       bExceptionNumber is the exception number to raise
*       ErrorCode is the optional error code for certain exceptions
*   Returns:
*       0
******************************************************************************/
// Assume stack is 32-bit in pmode

int CPU_Exception( BYTE bExceptionNumber, DWORD ErrorCode )
{
    ASSERT(bExceptionNumber < 32);

    CPU_Interrupt( bExceptionNumber );

    if( fHaveErrorCode[bExceptionNumber]==TRUE )
    {
        if( pMonitor->GDT[ pMonitor->guestTSS.ss >> 3 ].size32 )
        {
            // 32 bit stack selector
            CPU_PushDword( ErrorCode );
        }
        else
        {
            // 16 bit stack selector
            CPU_PushWord( LOWORD(ErrorCode) );
        }
    }
    return( 0 );
}


/******************************************************************************
*   void TranslateVmGdtToMonitorGdt()                                         *
*******************************************************************************
*   Copies every VM GDT entry into the master monitor GDT table and changes
*   privilege level from 0 to 1
******************************************************************************/
void TranslateVmGdtToMonitorGdt()
{
    DWORD count;
    ASSERT(CPU.pGDT);

    // Copy complete GDT table from VM into our monitor working GDT

    RtlCopyMemory( (void *)pMonitor->GDT, (void *)CPU.pGDT, CPU.guestGDT.limit + 1);

    // Fix-up privilege levels of code and data selectors

    count = (CPU.guestGDT.limit + 1) / sizeof(TGDT_Gate);
    do
    {
        if(pMonitor->GDT[count].present==TRUE)
            if(pMonitor->GDT[count].type & (1<<4))      // Bit 12 of GDT flags code/data
                    pMonitor->GDT[count].dpl |= 1;      //  selectors

    } while( --count );
}


/******************************************************************************
*   void TranslateSingleVmGdtToMonitorGdt()                                   *
*******************************************************************************
*   Copies a single VM GDT entry and possibly changes permissions for code
*   or data descriptors.
******************************************************************************/
void TranslateSingleVmGdtToMonitorGdt( DWORD dwSel )
{
    ASSERT(dwSel < CPU.guestGDT.limit);

    pMonitor->GDT[dwSel >> 3] = CPU.pGDT[dwSel >> 3];

    // Fix up privilege level for code/data selector

    if(pMonitor->GDT[dwSel >> 3].present==TRUE)
        if(pMonitor->GDT[dwSel >> 3].type & (1<<4))      // Bit 12 of GDT flags code/data
                pMonitor->GDT[dwSel >> 3].dpl |= 1;      //  selectors
}


/******************************************************************************
*                                                                             *
*   void LoadCR( DWORD cr, DWORD value )                                      *
*                                                                             *
*******************************************************************************
*
*   Loads one of the control register with a value.
*
*   Where:
*       cr is the control register number
*       value is the value to load
*
******************************************************************************/
void LoadCR( DWORD cr, DWORD value )
{
    int count;

    // Switch based on which control register we are loading
    switch( cr )
    {
        case 0:     // CR 0

            if( ((CPU.cr0 & BITMASK(CR0_PE_BIT))==0) && ((value & BITMASK(CR0_PE_BIT))!=0) )
            {
                //=============================================================================
                //              GOING FROM REAL MODE INTO PROTECTED MODE
                //=============================================================================

                ASSERT((pMonitor->guestTSS.eflags & VM_MASK)!=0);
                ASSERT(CPU.CPU_mode==CPU_REAL_MODE);

                //------------------------------------------------------------
                // Copy guest GDT into monitor master GDT, changing all selectors for code and
                // data from ring-0 to ring-1

                TranslateVmGdtToMonitorGdt();

                // Set up temporary code and data selectors that will briefly act
                // as real segments that were not yet reloaded by the guest code

                // They are all 16-bit, ring-1 selectors

                SetGDT(&pMonitor->GDT[ VM_SEL_REAL_ES >> 3 ],
                        (DWORD) pMonitor->guestTSS.es << 4,
                        0xFFFF, DESC_TYPE_DATA, FALSE);

                SetGDT(&pMonitor->GDT[ VM_SEL_REAL_CS >> 3 ],
                        (DWORD) pMonitor->guestTSS.cs << 4,
                        0xFFFF, DESC_TYPE_EXEC, FALSE);

                SetGDT(&pMonitor->GDT[ VM_SEL_REAL_SS >> 3 ],
                        (DWORD) pMonitor->guestTSS.ss << 4,
                        0xFFFF, DESC_TYPE_DATA, FALSE);

                SetGDT(&pMonitor->GDT[ VM_SEL_REAL_DS >> 3 ],
                        (DWORD) pMonitor->guestTSS.ds << 4,
                        0xFFFF, DESC_TYPE_DATA, FALSE);

                SetGDT(&pMonitor->GDT[ VM_SEL_REAL_FS >> 3 ],
                        (DWORD) pMonitor->guestTSS.fs << 4,
                        0xFFFF, DESC_TYPE_DATA, FALSE);

                SetGDT(&pMonitor->GDT[ VM_SEL_REAL_GS >> 3 ],
                        (DWORD) pMonitor->guestTSS.gs << 4,
                        0xFFFF, DESC_TYPE_DATA, FALSE);

                // Set new selectors that are to be used in the task structure

                pMonitor->guestTSS.cs = VM_SEL_REAL_CS;
                pMonitor->guestTSS.ds = VM_SEL_REAL_DS;
                pMonitor->guestTSS.es = VM_SEL_REAL_ES;
                pMonitor->guestTSS.fs = VM_SEL_REAL_FS;
                pMonitor->guestTSS.gs = VM_SEL_REAL_GS;
                pMonitor->guestTSS.ss = VM_SEL_REAL_SS;

                // We are not any more executing host in 'real' mode, but PM

                CPU.fMetaInvalidate = TRUE;             // Invalidate all meta pages
                SetCpuMode( CPU_KERNEL_MODE );          // We are in the VM kernel p-mode
            }
            else
            if( ((CPU.cr0 & BITMASK(CR0_PE_BIT))==1) && ((value & BITMASK(CR0_PE_BIT))==0) )
            {
                //=============================================================================
                //              GOING FROM PROTECTED MODE BACK INTO REAL MODE
                //=============================================================================

                ASSERT((pMonitor->guestTSS.eflags & VM_MASK)==0);
                ASSERT(CPU.CPU_mode==CPU_KERNEL_MODE);

                // Disable and delete all system regions that were registered
                // to trap (gdt, idt,...)

//                DisableSystemRegions();

                // Revert segment regs to their real-mode values.  The best we can do
                // is to set them at their base address.  Only segment registers that actually
                // changed are modified back.

                if( CPU.fSelPMode[0] ) pMonitor->guestTSS.es = (WORD) ((GetGDTBase(&pMonitor->GDT[ pMonitor->guestTSS.es >> 3 ]) >> 4) & 0xFFFF);
                if( CPU.fSelPMode[1] ) pMonitor->guestTSS.cs = (WORD) ((GetGDTBase(&pMonitor->GDT[ pMonitor->guestTSS.cs >> 3 ]) >> 4) & 0xFFFF);
                if( CPU.fSelPMode[2] ) pMonitor->guestTSS.ss = (WORD) ((GetGDTBase(&pMonitor->GDT[ pMonitor->guestTSS.ss >> 3 ]) >> 4) & 0xFFFF);
                if( CPU.fSelPMode[3] ) pMonitor->guestTSS.ds = (WORD) ((GetGDTBase(&pMonitor->GDT[ pMonitor->guestTSS.ds >> 3 ]) >> 4) & 0xFFFF);
                if( CPU.fSelPMode[4] ) pMonitor->guestTSS.fs = (WORD) ((GetGDTBase(&pMonitor->GDT[ pMonitor->guestTSS.fs >> 3 ]) >> 4) & 0xFFFF);
                if( CPU.fSelPMode[5] ) pMonitor->guestTSS.gs = (WORD) ((GetGDTBase(&pMonitor->GDT[ pMonitor->guestTSS.gs >> 3 ]) >> 4) & 0xFFFF);

                // We are now executing back in `real' mode

                CPU.fMetaInvalidate = TRUE;             // Invalidate all meta pages
                SetCpuMode( CPU_REAL_MODE );            // We are in real mode
            }

            if( !PAGING_ENABLED && ((value & BITMASK(CR0_PG_BIT))!=0) )
            {
                //=============================================================================
                //              ENABLE PAGING
                //=============================================================================
                ASSERT(0);

                EnablePaging();                         // Activate and start using paging
                CPU.fMetaInvalidate = TRUE;             // Invalidate all meta pages
            }
            else
            if( PAGING_ENABLED && ((value & BITMASK(CR0_PG_BIT))==0) )
            {
                //=============================================================================
                //              DISABLE PAGING
                //=============================================================================
                ASSERT(0);

                DisablePaging();                        // Deactivate and stop using paging
                CPU.fMetaInvalidate = TRUE;             // Invalidate all meta pages
            }

            // Store the new value into a virtual CPU register
            CPU.cr0 = value;
        break;

        case 3:     // CR 3 - PAGE DIRECTORY PHYSICAL ADDRESS (PDBR)
            // Register system region
//            RenewSystemRegion( SYSREG_PD, (DWORD)dwVmTSSBase, dwVmTSSLimit );

            CPU.cr3 = value;
            if( PAGING_ENABLED )
            {
                // ???
                CPU.fMetaInvalidate = TRUE;             // Invalidate all meta pages
            }
        break;

        default:    // All other control registers are illegal to access

            ASSERT(0);
    }
}

