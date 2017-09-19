/******************************************************************************
*                                                                             *
*   Module:     ProtectedInstructions.c                                       *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       4/28/2000                                                     *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

    Instructions that can not be executed natively are virtualized in this
    file.

    They should return RETURN_ADVANCE_EIP if the eip should be advanced,
    or RETURN_KEEP_EIP if the eip should not be advanced.
    (as example, when virtualized instruction explicitly change eip).

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 4/28/2000  Original                                             Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Include Files                                                             *
******************************************************************************/
#include <strmini.h>                // Include driver level C functions

#include <ntddk.h>                  // Include ddk function header file

#include "Vm.h"                     // Include root header file

#include "Scanner.h"                // Include scanner header file

#include "DisassemblerInterface.h"  // Include disassembler header file

/******************************************************************************
*                                                                             *
*   Global Variables                                                          *
*                                                                             *
******************************************************************************/

#if 1
#undef Breakpoint
#define Breakpoint
#endif

/******************************************************************************
*                                                                             *
*   External functions                                                        *
*                                                                             *
******************************************************************************/

extern DWORD fn_iret(DWORD flags, BYTE bModrm);

/******************************************************************************
*                                                                             *
*   Local Defines, Variables and Macros                                       *
*                                                                             *
******************************************************************************/

extern DWORD CurrentInstrLen;

#define RETURN_ADVANCE_EIP              0
#define RETURN_KEEP_EIP                 1

typedef DWORD (*TProtected)(DWORD flags, BYTE bModrm);

/******************************************************************************
*                                                                             *
*   Functions                                                                 *
*                                                                             *
******************************************************************************/
/*
    These virtual instructions are called from DisassemblerForVirtualization.c
*/

static DWORD fn_native(DWORD flags, BYTE bModrm)
{
    // This one is error-catcher
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_cli(DWORD flags, BYTE bModrm)
{
//    INFO(("Virtual interrupts disabled"));
    CPU.eflags &= ~IF_MASK;
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_clts(DWORD flags, BYTE bModrm)
{
    CPU.cr0 &= ~BITMASK(CR0_TS_BIT);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_hlt(DWORD flags, BYTE bModrm)
{

    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_inb(DWORD flags, BYTE bModrm)       // IN AL,Ib
{
    BYTE bPort;
    DWORD value;

Breakpoint;

    bPort = VmGetNextBYTE();
    value = (*Device.IO[ Device.IOIndexMap[bPort] ].pInCallback)(bPort, 1);
    *(BYTE *)&pMonitor->guestTSS.eax = (BYTE)(value & 0xFF);

    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_inv(DWORD flags, BYTE bModrm)       // IN eAX,Ib (opsize)
{
    BYTE bPort;
    DWORD value;

Breakpoint;

    bPort = VmGetNextBYTE();

    if( flags & DIS_DATA32 )
    {
        // 32 bit data size

        value = (*Device.IO[ Device.IOIndexMap[bPort] ].pInCallback)(bPort, 4);
        pMonitor->guestTSS.eax = value;
    }
    else
    {
        // 16 bit data size

        value = (*Device.IO[ Device.IOIndexMap[bPort] ].pInCallback)(bPort, 2);
        *(WORD *)&pMonitor->guestTSS.eax = LOWORD(value);
    }

    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_inal(DWORD flags, BYTE bModrm)      // IN AL,DX
{
    WORD wPort;
    DWORD value;

Breakpoint;

    wPort = LOWORD(pMonitor->guestTSS.edx);
    value = (*Device.IO[ Device.IOIndexMap[wPort] ].pInCallback)(wPort, 1);
    *(BYTE *)&pMonitor->guestTSS.eax = (BYTE)(value & 0xFF);

    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_inax(DWORD flags, BYTE bModrm)      // IN eAX,DX (opsize)
{
    WORD wPort;
    DWORD value;

Breakpoint;

    wPort = LOWORD(pMonitor->guestTSS.edx);

    if( flags & DIS_DATA32 )
    {
        // 32 bit data size

        value = (*Device.IO[ Device.IOIndexMap[wPort] ].pInCallback)(wPort, 4);
        pMonitor->guestTSS.eax = value;
    }
    else
    {
        // 16 bit data size

        value = (*Device.IO[ Device.IOIndexMap[wPort] ].pInCallback)(wPort, 2);
        *(WORD *)&pMonitor->guestTSS.eax = LOWORD(value);
    }

    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_insb(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_insv(DWORD flags, BYTE bModrm)
{
//    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_int_n(DWORD flags, BYTE bModrm)
{
    BYTE bIntNumber;

    // Advance eip so the CPU_Interrupt() can push following instruction
    // on the stack.  Also, do not advance eip on return.  Int n instruction
    // is always 2 bytes long.

    pMonitor->guestTSS.eip += 2;
    bIntNumber = VmGetNextBYTE();
    CPU_Interrupt(bIntNumber);

    return( RETURN_KEEP_EIP );
}

//=============================================================================
static DWORD fn_into(DWORD flags, BYTE bModrm)
{
    ASSERT(0);

    // Advance eip manually to push it on the stack if the interrupt should be
    // executed, or simply proceed. INTO is 1 byte long.

    pMonitor->guestTSS.eip += 1;

    // Generate interrupt 4 only if overflow flag is set

    if( pMonitor->guestTSS.eflags & BITMASK(OF_BIT) )
    {
        CPU_Exception(EXCEPTION_INTO, 0);
    }

    return( RETURN_KEEP_EIP );
}

//=============================================================================
static DWORD fn_invd(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_invlpg(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
// Load Global Descriptor Table
//=============================================================================
static DWORD fn_lgdt(DWORD flags, BYTE bModrm)
{
    DWORD dwVmGDT;
    TDescriptor *pGDT_descriptor;

    // Get the address of the gdt table to load

    dwVmGDT = DecodeMemoryPointer( &flags, bModrm );

    // Translate the VM address to driver address

    pGDT_descriptor = (TDescriptor *) VmLinearToDriverLinear(dwVmGDT);
    ASSERT(pGDT_descriptor);

    // Fetch base address and limit of the new VM GDT descriptor
    // and store in our virtual guest processor state structure

    CPU.guestGDT.base  = pGDT_descriptor->base;
    CPU.guestGDT.limit = pGDT_descriptor->limit;
    ASSERT(CPU.guestGDT.limit);
    ASSERT(CPU.guestGDT.limit < GDT_SEL_VM_TOP);

    // In 16-bit operand mode, zero out upper 8 bits of the base address
    if( (flags & DIS_DATA32)==0 )
        CPU.guestGDT.base &= 0x00FFFFFF;

    // Get the driver pointer to actual GDT table
    CPU.pGDT = (TGDT_Gate *) VmLinearToDriverLinear(CPU.guestGDT.base);
    ASSERT(CPU.pGDT);

    // Register system region
    RenewSystemRegion( SYSREG_GDT, CPU.guestGDT.base, CPU.guestGDT.limit + 1 );

    // Copy all vm gdt entries into master monitor gdt table
    TranslateVmGdtToMonitorGdt();

    INFO(("GDT loaded: %08X Limit %04X", CPU.guestGDT.base, CPU.guestGDT.limit));

    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
// Load Interrupt Descriptor Table
//=============================================================================
static DWORD fn_lidt(DWORD flags, BYTE bModrm)
{
    DWORD dwVmIDT;
    TDescriptor *pIDT_descriptor;

    // Get the linear VM address of the IDT table to load

    dwVmIDT = DecodeMemoryPointer( &flags, bModrm );

    // Translate the VM address to driver address

    pIDT_descriptor = (TDescriptor *) VmLinearToDriverLinear(dwVmIDT);
    ASSERT(pIDT_descriptor);

    // Fetch base address and limit of the new VM IDT descriptor
    // and store in our virtual guest processor state structure

    CPU.guestIDT.base  = pIDT_descriptor->base;
    CPU.guestIDT.limit = pIDT_descriptor->limit;
    ASSERT(CPU.guestIDT.limit < 256 * sizeof(TIDT_Gate));

    // In 16-bit operand mode, zero out upper 8 bits of the base address
    if( (flags & DIS_DATA32)==0 )
        CPU.guestIDT.base &= 0x00FFFFFF;

    // Get the driver pointer to actual IDT table
    CPU.pIDT = (TIDT_Gate *) VmLinearToDriverLinear(CPU.guestIDT.base);
    ASSERT(CPU.pIDT);

    // Register system region
    // TODO: Why do we need to track writes to VM IDT ???
    //    RenewSystemRegion( SYSREG_IDT, CPU.guestIDT.base, CPU.guestIDT.limit + 1 );

    INFO(("IDT loaded: %08X Limit %04X", CPU.guestIDT.base, CPU.guestIDT.limit));

    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
// Load Task Register; also set up TSS structure
//=============================================================================
static DWORD fn_ltr(DWORD flags, BYTE bModrm)
{
    WORD wTask;
    DWORD dwVmTSSBase;
    DWORD dwVmTSSLimit;

    ASSERT(0);

    // Get the GDT entry describing the task state structure

    wTask = DecodeEw( &flags, bModrm );

    // Store the guest Task Register away since we will be using
    // our own monitor task state structure instead

    CPU.guestTR = wTask;

    INFO(("TR loaded: %04X", CPU.guestTR));

    dwVmTSSBase = GetGDTBaseLimit(&dwVmTSSLimit, &pMonitor->GDT[ wTask >> 3 ]);

    // Register system region
    // TODO: Why do we need to track writes to VM TSS ???
    //     RenewSystemRegion( SYSREG_TSS, dwVmTSSBase, dwVmTSSLimit );

    // Get the driver pointer to the virtual TSS
    CPU.pTSS = (TSS_Struct *) VmLinearToDriverLinear(dwVmTSSBase);
    ASSERT(CPU.pTSS);

    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_mov_to_cr(DWORD flags, BYTE bModrm)
{
    DWORD cr, value;

    ASSERT((bModrm & 0xC0)==0xC0);

    // Get new value from a general-purpose register ( Rm field )

    cr = (bModrm >> 3) & 7;
    value = *(((DWORD *)&pMonitor->guestTSS.eax) + (bModrm & 0x7) );

    LoadCR( cr, value );

    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_lldt(DWORD flags, BYTE bModrm)
{
    WORD wSel;
    ASSERT(0);

    wSel = DecodeEw( &flags, bModrm );
    if( wSel == 0 )
    {
        pMonitor->guestLDT = wSel;
        return( RETURN_ADVANCE_EIP );
    }

    if( CPU.CPU_mode != CPU_REAL_MODE )
    {
        if( CPU.CPU_mode==CPU_KERNEL_MODE )
        {
            if( (wSel & SEL_TI_MASK)==0 )
            {
                if( pMonitor->GDT[wSel >> 3].present == TRUE )
                {
                    if( pMonitor->GDT[wSel >> 3].type==DESC_TYPE_LDT )
                    {
                        pMonitor->guestLDT = wSel;

                        return( RETURN_ADVANCE_EIP );
                    }
                    else
                        CPU_Exception( EXCEPTION_GPF, wSel & 0xFFFC );
                }
                else
                    CPU_Exception( EXCEPTION_SEG_NOT_PRESENT, wSel & 0xFFFC );
            }
            else
                CPU_Exception( EXCEPTION_GPF, wSel & 0xFFFC );
        }
        else
            CPU_Exception( EXCEPTION_GPF, 0 );
    }
    else
        CPU_Exception( EXCEPTION_INVALID_OPCODE, 0 );

    return( RETURN_KEEP_EIP );
}

//=============================================================================
static DWORD fn_lmsw(DWORD flags, BYTE bModrm)
{
    DWORD value;

    value = (DWORD) DecodeEw( &flags, bModrm );
    if( (CPU.cr0 & BITMASK(CR0_PE_BIT)) && ((value & BITMASK(CR0_PE_BIT))==0) )
        WARNING(("Used lmsw to switch back to real mode"));

    // This instruction can load only the low nibble of CR0: PE, MP, EM and TS bits
    value &= 0x0F;
    value |= CPU.cr0 & ~0x0F;

    LoadCR( 0, value );

    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_mov_to_dr(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_mov_from_dr(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_outb(DWORD flags, BYTE bModrm)      // Ib,AL
{
    BYTE bPort;
    DWORD value;

Breakpoint;

    bPort = VmGetNextBYTE();
    value = pMonitor->guestTSS.eax & 0xFF;
    (*Device.IO[ Device.IOIndexMap[bPort] ].pOutCallback)(bPort, value, 1);

    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_outv(DWORD flags, BYTE bModrm)      // Ib,eAX (opsize)
{
    BYTE bPort;
    DWORD value;

Breakpoint;

    bPort = VmGetNextBYTE();

    if( flags & DIS_DATA32 )
    {
        // 32 bit data size

        value = pMonitor->guestTSS.eax;
        (*Device.IO[ Device.IOIndexMap[bPort] ].pOutCallback)(bPort, value, 4);
    }
    else
    {
        // 16 bit data size

        value = LOWORD(pMonitor->guestTSS.eax);
        (*Device.IO[ Device.IOIndexMap[bPort] ].pOutCallback)(bPort, value, 2);
    }

    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_outal(DWORD flags, BYTE bModrm)     // DX,AL
{
    WORD wPort;
    DWORD value;

Breakpoint;

    wPort = LOWORD(pMonitor->guestTSS.edx);
    value = pMonitor->guestTSS.eax & 0xFF;

    (*Device.IO[ Device.IOIndexMap[wPort] ].pOutCallback)(wPort, value, 1);

    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_outax(DWORD flags, BYTE bModrm)     // DX,eAX
{
    WORD wPort;
    DWORD value;

Breakpoint;

    wPort = LOWORD(pMonitor->guestTSS.edx);

    if( flags & DIS_DATA32 )
    {
        // 32 bit data size

        value = pMonitor->guestTSS.eax;
        (*Device.IO[ Device.IOIndexMap[wPort] ].pOutCallback)(wPort, value, 4);
    }
    else
    {
        // 16 bit data size

        value = LOWORD(pMonitor->guestTSS.eax);
        (*Device.IO[ Device.IOIndexMap[wPort] ].pOutCallback)(wPort, value, 2);
    }

    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_outsb(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_outsv(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_rdmsr(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_rdmpc(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_rdtsc(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_sti(DWORD flags, BYTE bModrm)
{
//    INFO(("Virtual interrupts enabled"));
    CPU.eflags |= IF_MASK;
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_wbinvd(DWORD flags, BYTE bModrm)
{
    // Serializing instruction that flushes internal and external caches
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_wrmsr(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_call_near(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_KEEP_EIP );
}

//=============================================================================
static DWORD fn_call_far(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_KEEP_EIP );
}

//=============================================================================
static DWORD fn_call_neari(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_KEEP_EIP );
}

//=============================================================================
static DWORD fn_call_fari(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_KEEP_EIP );
}

//=============================================================================
static DWORD fn_cpuid(DWORD flags, BYTE bModrm)
{
    TRACE(("CPUID instruction"));
    ASSERT(0);

    switch( pMonitor->guestTSS.eax )
    {
        case 0:
                pMonitor->guestTSS.eax = CPU.cpuid[0][0];
                pMonitor->guestTSS.ebx = CPU.cpuid[0][1];
                pMonitor->guestTSS.ecx = CPU.cpuid[0][2];
                pMonitor->guestTSS.edx = CPU.cpuid[0][3];
            break;

        case 1:
                pMonitor->guestTSS.eax = CPU.cpuid[1][0];
                pMonitor->guestTSS.ebx = CPU.cpuid[1][1];
                pMonitor->guestTSS.ecx = CPU.cpuid[1][2];
                pMonitor->guestTSS.edx = CPU.cpuid[1][3];
            break;

        case 2:
                pMonitor->guestTSS.eax = CPU.cpuid[2][0];
                pMonitor->guestTSS.ebx = CPU.cpuid[2][1];
                pMonitor->guestTSS.ecx = CPU.cpuid[2][2];
                pMonitor->guestTSS.edx = CPU.cpuid[2][3];
            break;

        default:
            WARNING(("Invalid eax of %X for cpuid", pMonitor->guestTSS.eax));
    }

    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_jmp_near(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_KEEP_EIP );
}

//=============================================================================
static DWORD fn_jmp_near2(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_KEEP_EIP );
}

//=============================================================================
static DWORD fn_jmp_neari(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_KEEP_EIP );
}

//=============================================================================
static DWORD fn_jmp_far(DWORD flags, BYTE bModrm)
{
    WORD wSel;

    // Normally, this instruction would be executed single-step, but since
    // the Phoenix system BIOS does a weird thing and enters protected mode,
    // but never reloads CS register, we need to virtualize this behaviour.
    //
    // EA cd/cp     Ap       -> ptr16:16/ptr16:32       (opsize)(seg)

    // We need to virtualize this instruction since the CS can still be segment
    // even when PE=1

    // This instruction effectively flushes the CS selector into the right mode

    if( CPU.CPU_mode==CPU_KERNEL_MODE )
    {
        CPU.fSelPMode[ 1 ] = TRUE;

        if( flags & DIS_DATA32 )
            pMonitor->guestTSS.eip = VmGetNextDWORD();          // 32-bit jump
        else
            pMonitor->guestTSS.eip = (DWORD) VmGetNextWORD();   // 16-bit jump

        wSel = VmGetNextWORD();

        // Run ring 0 guest in ring 1
        ASSERT((wSel & SEL_TI_MASK)==0);        // Make sure it is GDT
        wSel |= SEL_RPL1_MASK;

        pMonitor->guestTSS.cs = wSel;

        if( wSel >= GDT_SEL_VM_TOP )
        {
            ERROR(("VM is using selector %04X. Increase GDT_MAX and recompile.", wSel));
            ASSERT(0);
        }
    }
    else    // Real or v86 mode
    {
        CPU.fSelPMode[ 1 ] = FALSE;
        pMonitor->guestTSS.eip = (DWORD) VmGetNextWORD();
        pMonitor->guestTSS.cs = VmGetNextWORD();
    }

    return( RETURN_KEEP_EIP );
}

//=============================================================================
static DWORD fn_jmp_fari(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_KEEP_EIP );
}

//=============================================================================
static DWORD fn_lar(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}


//=============================================================================
// Loading of selector:register value pair: lds, les, lfs, lgs, lss
// Even if we let those run natively, they will GPF since the RPL of the
// selector value is 0
//=============================================================================
#define SEL_INDEX_ES        0
#define SEL_INDEX_CS        1
#define SEL_INDEX_SS        2
#define SEL_INDEX_DS        3
#define SEL_INDEX_FS        4
#define SEL_INDEX_GS        5

static void LoadSelRegPair( DWORD indexSel, DWORD flags, BYTE bModrm )
{
    DWORD dwVmSelOfs, *pSelOfs, dwOffset, reg;
    WORD wSel;

    // These are virtualized in pmode only!
    ASSERT(CPU.CPU_mode==CPU_KERNEL_MODE);

    // Get the address of the SEL:OFS to load
    dwVmSelOfs = DecodeMemoryPointer( &flags, bModrm );

    // Translate the VM address to driver address

    pSelOfs = (DWORD *) VmLinearToDriverLinear(dwVmSelOfs);
    ASSERT(pSelOfs);

    reg = (bModrm >> 3) & 7;        // Which register to load (offset)

    // Fetch the new selector/offset and store them

    if( flags & DIS_DATA32 )        // 32 bit data
    {
        dwOffset = *pSelOfs++;
        *(((DWORD *)&pMonitor->guestTSS.eax) + reg ) = dwOffset;
    }
    else                            // 16 bit data
    {
        dwOffset = (DWORD) *((WORD *)pSelOfs)++;
        *(WORD *)(((DWORD *)&pMonitor->guestTSS.eax) + reg ) = (WORD) dwOffset;
    }

    wSel = *(WORD *)pSelOfs;
    wSel |= SEL_RPL1_MASK;          // Make ring0 become ring 1
    *(((DWORD *)&pMonitor->guestTSS.es) + indexSel) = wSel;

    // TODO: Do we really need to keep track of selector mode?  CPU.fSelPMode[]
    CPU.fSelPMode[ indexSel ] = TRUE;
}

//=============================================================================
static DWORD fn_lds(DWORD flags, BYTE bModrm)
{
    LoadSelRegPair(SEL_INDEX_DS, flags, bModrm);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_les(DWORD flags, BYTE bModrm)
{
    LoadSelRegPair(SEL_INDEX_ES, flags, bModrm);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_lfs(DWORD flags, BYTE bModrm)
{
    LoadSelRegPair(SEL_INDEX_FS, flags, bModrm);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_lgs(DWORD flags, BYTE bModrm)
{
    LoadSelRegPair(SEL_INDEX_GS, flags, bModrm);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_lss(DWORD flags, BYTE bModrm)
{
    LoadSelRegPair(SEL_INDEX_SS, flags, bModrm);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_lsl(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_mov_from_cr(DWORD flags, BYTE bModrm)       // Rd,Cd
{
    DWORD value;
    DWORD Cr;

    ASSERT((bModrm & 0xC0)==0xC0);

    Cr = (bModrm >> 3) & 7;         // Reg field is control register

    switch( Cr )
    {
        case 0:
            value = CPU.cr0;
        break;

        case 3:
            value = CPU.cr3;
        break;

        default:
            ASSERT(0);
    }

    *(((DWORD *)&pMonitor->guestTSS.eax) + (bModrm & 0x7) ) = value;

    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_mov_to_seg(DWORD flags, BYTE bModrm)
{
    WORD wNewValue;
    DWORD indexSeg;

    wNewValue = DecodeEw( &flags, bModrm );
    indexSeg = (bModrm >> 3) & 7;

    if( CPU.CPU_mode==CPU_KERNEL_MODE )
    {
        if( wNewValue >= GDT_SEL_VM_TOP )
        {
            ERROR(("VM is using selector %04X. Increase GDT_MAX and recompile.", wNewValue));
            ASSERT(0);
        }

        wNewValue |= SEL_RPL1_MASK;             // Make ring0 become ring 1

        CPU.fSelPMode[ indexSeg ] = TRUE;
    }
    else
        CPU.fSelPMode[ indexSeg ] = FALSE;

    *(((DWORD *)&pMonitor->guestTSS.es) + indexSeg) = wNewValue;

    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_mov_from_seg(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_pop_ds(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_pop_es(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_pop_fs(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_pop_ss(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_pop_gs(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_popf(DWORD flags, BYTE bModrm)
{
    // Pop flags word or dword depending on the stack segment/selector

    switch( CPU.CPU_mode )
    {
        case CPU_REAL_MODE:
        case CPU_V86_MODE:
                CPU.eflags = CPU_PopWord();
                pMonitor->guestTSS.eflags = EFLAGS_SET_CONSTANTS(CPU.eflags & EFLAGS_ACTIVE_MASK) | VM_MASK | IF_MASK;
            break;
        case CPU_KERNEL_MODE:
            if( flags & DIS_DATA32 )        // POPFD
                CPU.eflags = CPU_PopDword();
            else                            // POPF
                CPU.eflags = (CPU.eflags & ~0xFFFF) | (CPU_PopWord() & 0xFFFF);
            pMonitor->guestTSS.eflags = EFLAGS_SET_CONSTANTS(CPU.eflags & EFLAGS_ACTIVE_MASK) | IF_MASK;
            break;
    }
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_push_cs(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_push_ss(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_push_ds(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_push_es(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_push_fs(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_push_gs(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_pushf(DWORD flags, BYTE bModrm)
{
    DWORD dwFlags;

    // Push flags word or dword depending on the stack segment/selector

    dwFlags = LOWORD((pMonitor->guestTSS.eflags & EFLAGS_ACTIVE_MASK) | (CPU.eflags & ~EFLAGS_ACTIVE_MASK));

    switch( CPU.CPU_mode )
    {
        case CPU_REAL_MODE:
        case CPU_V86_MODE:
                CPU_PushWord((WORD) (dwFlags & 0xFFFF));
            break;
        case CPU_KERNEL_MODE:
            if( flags & DIS_DATA32 )        // PUSHFD
                CPU_PushDword(dwFlags);
            else                            // PUSHF
                CPU_PushWord((WORD) (dwFlags & 0xFFFF));
            break;
    }
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_ret_near(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_KEEP_EIP );
}

//=============================================================================
static DWORD fn_ret_far(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_KEEP_EIP );
}

//=============================================================================
static DWORD fn_ret_near_pop(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_KEEP_EIP );
}

//=============================================================================
static DWORD fn_ret_far_pop(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_KEEP_EIP );
}

//=============================================================================
static DWORD fn_sgdt(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_sidt(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_sldt(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_smsw(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_str(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_verr(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_verw(DWORD flags, BYTE bModrm)
{
    ASSERT(0);
    return( RETURN_ADVANCE_EIP );
}

//=============================================================================

/******************************************************************************
*                                                                             *
*   Array of pointers to functions above                                      *
*                                                                             *
******************************************************************************/

TProtected Protected[] = {
    fn_native,          //  0x00
    fn_cli,             //  0x01      // FA           -
    fn_clts,            //  0x02      // 0F 06        -
    fn_hlt,             //  0x03      // F4           -
    fn_inb,             //  0x04      // E4 ib        Al, Ib   -> Al, imm8
    fn_inv,             //  0x05      // E5 ib        eAX, Ib  -> AX/EAX, imm8            (opsize)
    fn_inal,            //  0x06      // EC           AL, DX   -> AL, DX
    fn_inax,            //  0x07      // ED           eAX, DX  -> AX/EAX, DX              (opsize)
    fn_insb,            //  0x08      // 6C           Yb, DX   -> ES:(E)DI, DX     (seg)  (opsize)    (rep)
    fn_insv,            //  0x09      // 6D           Yv, DX   -> ES:(E)DI, DX     (seg)  (opsize)    (rep)
    fn_int_n,           //  0x0A  // CD ib        Ib
    fn_into,            //  0x0B  // CE           -
    fn_invd,            //  0x0C  // 0F 08        -
    fn_invlpg,          //  0x0D  // 0F 01/7      M        -> linear address   (ignore instruction)
    fn_lgdt,            //  0x0E  // 0F 01/2      Ms       -> linear address
    fn_lidt,            //  0x0F  // 0F 01/3      Ms       -> linear address
    fn_lldt,            //  0x10  // 0F 00/2      Ew       -> rm16 selector
    fn_lmsw,            //  0x11  // 0F 01/6      Ew       -> rm16
    fn_ltr,             //  0x12  // 0F 00/3      Ew       -> rm16
    fn_mov_to_cr,       //  0x13  // 0F 22/r      Cd, Rd   -> CR,r32
    fn_mov_to_dr,       //  0x14  // 0F 23/r      Dd, Rd   -> DR,r32
    fn_mov_from_dr,     //  0x15  // 0F 21/r      Rd, Dd   -> r32,DR
    fn_outb,            //  0x16  // E6 ib        Ib, AL   -> imm8,AL
    fn_outv,            //  0x17  // E7 ib        Ib, eAX  -> imm8,AX/EAX             (opsize)
    fn_outal,           //  0x18  // EE           DX, AL   -> DX, AL
    fn_outax,           //  0x19  // EF           DX, eAX  -> DX, AX/EAX              (opsize)
    fn_outsb,           //  0x1A  // 6E           DX, Xb   -> DX, DS:(E)SI     (seg)  (opsize)    (rep)
    fn_outsv,           //  0x1B  // 6F           DX, Xv   -> DX, DS:(E)SI     (seg)  (opsize)    (rep)
    fn_rdmsr,           //  0x1C  // 0F 32        -
    fn_rdmpc,           //  0x1D  // 0F 33        -
    fn_rdtsc,           //  0x1E  // 0F 31        -
    fn_sti,             //  0x1F  // FB           -
    fn_wbinvd,          //  0x20  // 0F 09        -
    fn_wrmsr,           //  0x21  // 0F 30        -
    fn_call_near,       //  0x22  // E8 cv/cd     Jv       -> rel16/rel32 disp        (opsize)
    fn_call_far,        //  0x23  // 9A cd/cp     Ap       -> ptr16:16/ptr16:32       (opsize)(seg)
    fn_call_neari,      //  0x24  // FF /2        Ev       -> rm16/rm32               (opsize)
    fn_call_fari,       //  0x25  // FF /3        Ep       -> m16:16/m16:32           (opsize)
    fn_cpuid,           //  0x26  // 0F A2        -
    fn_iret,            //  0x27  // CF           -           16 and 32 bit version   (opsize)
    fn_jmp_near,        //  0x28  // EB cb        Jb       -> rel8
    fn_jmp_near2,       //  0x29  // E9 cw/cd     Jv       -> rel16/rel32             (opsize)
    fn_jmp_neari,       //  0x2A  // FF /4        Ev       -> rm16/rm32               (opsize)
    fn_jmp_far,         //  0x2B  // EA cd/cp     Ap       -> ptr16:16/ptr16:32       (opsize)(seg)
    fn_jmp_fari,        //  0x2C  // FF /5        Ep       -> m16:16/m16:32           (opsize)
    fn_lar,             //  0x2D  // 0F 02 /r     Gv, Ew   -> r16/r32,rm16/rm32       (opsize)
    fn_lds,             //  0x2E  // C5 /r        Gv, Mp   -> DS:r16/DS:r32           (opsize)
    fn_les,             //  0x2F  // C4 /r        Gv, Mp   -> ES:r16/ES:r32           (opsize)
    fn_lfs,             //  0x30  // 0F B4 /r     Gv, Mp   -> FS:r16/FS:r32           (opsize)
    fn_lgs,             //  0x31  // 0F B5 /r     Gv, Mp   -> GS:r16/GS:r32           (opsize)
    fn_lss,             //  0x32  // 0F B2 /r     Gv, Mp   -> SS:r16/SS:r32           (opsize)
    fn_lsl,             //  0x33  // 0F 03 /r     Gv, Ew   -> r16/r32,rm16/rm32       (opsize)
    fn_mov_from_cr,     //  0x34  // 0F 20 /r     Rd, Cd   -> r32,CR
    fn_mov_to_seg,      //  0x35  // 8E /r        Sw, Ew   -> seg,rm16
    fn_mov_from_seg,    //  0x36  // 8C /r        Ew, Sw   -> rm16,seg
    fn_pop_ds,          //  0x37  // 1F           -
    fn_pop_es,          //  0x38  // 07           -
    fn_pop_fs,          //  0x39  // 0F A1        -
    fn_pop_ss,          //  0x3A  // 17           -
    fn_pop_gs,          //  0x3B  // 0F A9        -
    fn_popf,            //  0x3C  // 9D           -           16 and 32 bit version   (opsize)
    fn_push_cs,         //  0x3D  // 0E           -
    fn_push_ss,         //  0x3E  // 16           -
    fn_push_ds,         //  0x3F  // 1E           -
    fn_push_es,         //  0x40  // 06           -
    fn_push_fs,         //  0x41  // 0F A0        -
    fn_push_gs,         //  0x42  // 0F A8        -
    fn_pushf,           //  0x43  // 9C           -           16 and 32 bit version   (opsize)
    fn_ret_near,        //  0x44  // C3           -
    fn_ret_far,         //  0x45  // CB           -
    fn_ret_near_pop,    //  0x46  // C2 iw        Iw       -> imm16
    fn_ret_far_pop,     //  0x47  // CA iw        Iw       -> imm16
    fn_sgdt,            //  0x48  // 0F 01 /0     Ms       -> linear address
    fn_sidt,            //  0x49  // 0F 01 /1     Ms       -> linear address
    fn_sldt,            //  0x4A  // 0F 00 /0     Ew       -> rm16/rm32  *            (opsize)
    fn_smsw,            //  0x4B  // 0F 01 /4     Ew       -> rm16/rm32  *            (opsize)
    fn_str,             //  0x4C  // 0F 00 /1     Ew       -> rm16/rm32  *            (opsize)
    fn_verr,            //  0x4D  // 0F 00 /4     Ew       -> rm16
    fn_verw             //  0x4E      // 0F 00 /5     Ew       -> rm16
};
