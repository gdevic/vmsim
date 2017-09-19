/******************************************************************************
*                                                                             *
*   Module:     CPUTraps.c                                                    *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       3/28/2000                                                     *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the code for CPU virtualization.  CPU traps and
        faults are processed here.

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 3/28/2000  Original                                             Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Include Files                                                             *
******************************************************************************/
#include <strmini.h>                // Include driver level C functions

#include <ntddk.h>                  // Include ddk function header file

#include "Vm.h"                     // Include root header file

#include "Scanner.h"                // Include scanner defines

#include "DisassemblerInterface.h"  // Include disassembler interface header

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

typedef int (*pTrapFunction)(TMeta * pMeta);

static int Int0_DivideError(TMeta *);
static int Int1_Debug(TMeta *);
static int Int2_NMI(TMeta *);
static int Int3_Breakpoint(TMeta *);
static int Int4_Overflow(TMeta *);
static int Int5_BOUND(TMeta *);
static int Int6_InvalidOpcode(TMeta *);
static int Int7_DeviceNotAvailable(TMeta *);
static int Int8_DoubleFault(TMeta *);
static int Int9_CoprocessorSegmentOverrun(TMeta *);
static int Int10_InvalidTSS(TMeta *);
static int Int11_SegmentNotPresent(TMeta *);
static int Int12_StackFault(TMeta *);
static int Int13_GeneralProtectionFault(TMeta *);
static int Int14_PageFault(TMeta *);
static int Int16_FloatingPointError(TMeta *);
static int Int17_AlignmentCheck(TMeta *);
static int Int18_MachineCheck(TMeta *);
static int IntShouldNotHappen(TMeta *);

// 旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
// �  Define trap handlers for interrupts 0 - 31                              �
// 읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸

static pTrapFunction DispatchTable [32] =
{
    Int0_DivideError,
    Int1_Debug,
    Int2_NMI,
    Int3_Breakpoint,
    Int4_Overflow,
    Int5_BOUND,
    Int6_InvalidOpcode,
    Int7_DeviceNotAvailable,
    Int8_DoubleFault,
    Int9_CoprocessorSegmentOverrun,
    Int10_InvalidTSS,
    Int11_SegmentNotPresent,
    Int12_StackFault,
    Int13_GeneralProtectionFault,
    Int14_PageFault,
    IntShouldNotHappen,
    Int16_FloatingPointError,
    Int17_AlignmentCheck,
    Int18_MachineCheck,
    IntShouldNotHappen,
    IntShouldNotHappen,
    IntShouldNotHappen,
    IntShouldNotHappen,
    IntShouldNotHappen,
    IntShouldNotHappen,
    IntShouldNotHappen,
    IntShouldNotHappen,
    IntShouldNotHappen,
    IntShouldNotHappen,
    IntShouldNotHappen,
    IntShouldNotHappen,
    IntShouldNotHappen
};

/******************************************************************************
*                                                                             *
*   Functions                                                                 *
*                                                                             *
******************************************************************************/

/******************************************************************************
*                                                                             *
*   int ExecuteTrap( TMeta *pMeta, int nInt)                                  *
*                                                                             *
*******************************************************************************
*
*   This function takes care of traps and faults.
*
*   Where:
*       pMeta is a pointer to a current meta structure
*       nInt is the interrupt number (0-31)
*
*   Returns:
*       0 next instruction
*       1 execute instruction again single step
*
******************************************************************************/
int ExecuteTrap( TMeta *pMeta, int nInt)
{
    TSS_Struct *pTSS;

    ASSERT(nInt>=0);
    ASSERT(nInt<32);

    // Clear RF flag

    pMonitor->guestTSS.eflags &= ~RF_MASK;

    return ((*DispatchTable[nInt])(pMeta));
}


//=============================================================================
static int IntShouldNotHappen (TMeta * pMeta)
//=============================================================================
{
    TRACE(("IntShouldNotHappen"));
    ASSERT(0);
    return( 0 );
}

//=============================================================================
static int Int0_DivideError (TMeta * pMeta)
//=============================================================================
{
    TRACE(("Int0_DivideError"));
    ASSERT(0);
    return( 0 );
}

//=============================================================================
static int Int1_Debug (TMeta * pMeta)
// This breakpoint indicates one or more of several debug-exception conditions:
// (5-19 ref. vol. 3)
//=============================================================================
{
//    TRACE(("Int1_Debug"));

    // We returned from a single step

    // Clear TF flag so we proceed normally

    SetMainLoopState( stateNext, NULL );

    pMonitor->guestTSS.eflags &= ~TF_MASK;

    return( 0 );
}

//=============================================================================
static int Int2_NMI (TMeta * pMeta)
//=============================================================================
{
    TRACE(("Int2_NMI"));
    ASSERT(0);
    return( 0 );
}

//=============================================================================
static int Int3_Breakpoint (TMeta * pMeta)
//=============================================================================
{
    DWORD flags;
    BYTE bMeta;
    BYTE *pTarget;
    DWORD dwVmLinear;

    // Fix up the eip since it is advanced by the INT 3

    pMonitor->guestTSS.eip -= 1;

    // Get the meta byte

    dwVmLinear = CSIPtoVmLinear();
    bMeta = *(BYTE *)(pMeta->MemPage.linear + 4096 + (dwVmLinear & 0xFFF));

    // We had to process this instruction before!!!

    ASSERT(bMeta);

    // Bits [7:4] of the meta byte specify what we should do with this instruction

    switch( bMeta >> 4 )
    {
        case META_SINGLE_STEP:
                // Explicit single-step instructions (like RET) and
                // instructions that cross page boundaries we execute single-step

                SetMainLoopState( stateStep, NULL );

            break;

        case META_TERMINATING:
                // Terminating instructions we virtualize.

                pTarget = (BYTE *)CSIPtoDriverLinear();

                if( DisVirtualize( pTarget, dwVmLinear, NULL ) == 0 )
                {
                    // Advance eip to the next instruction... Otherwise, we
                    // repeat the same instruction.

                    // Position eip to the next instruction, even if we did not pick
                    // up all its arguments within the virtualization code

                    // TODO: What if PF is caused by instruction that was set to be
                    // terminating since it spans a page boundary, so the length may
                    // not be computed corerctly????

                    pMonitor->guestTSS.eip += bMeta & 0x0F;
                }
            break;

        case META_UNDEF:
                // Undefined (illegal) instructions cause #UD fault within the VM

                ASSERT(0);      // Not yet tested

                CPU_Exception( EXCEPTION_INVALID_OPCODE, 0 );

            break;

        case META_NATIVE:
                // Native instruction caused INT3?? It's a user breakpoint !
                // Generate int 3 within the VM; Ok, ok, we changed our mind and revert
                // eip to the instruction following the int 3
                pMonitor->guestTSS.eip += 1;

                // (-2): Software interrupt
                CPU_Exception( EXCEPTION_BREAKPOINT, 0 );
            break;

        default:        // Ooops !
                ASSERT(0);
            break;
    }

    return( 0 );
}

//=============================================================================
static int Int4_Overflow (TMeta * pMeta)
//=============================================================================
{
    TRACE(("Int4_Overflow"));
    ASSERT(0);
    return( 0 );
}

//=============================================================================
static int Int5_BOUND (TMeta * pMeta)
//=============================================================================
{
    TRACE(("Int5_BOUND"));
    ASSERT(0);
    return( 0 );
}

//=============================================================================
static int Int6_InvalidOpcode (TMeta * pMeta)
//=============================================================================
{
    TRACE(("Int6_InvalidOpcode"));
    ASSERT(0);
    return( 0 );
}

//=============================================================================
static int Int7_DeviceNotAvailable (TMeta * pMeta)
//=============================================================================
{
    TRACE(("Int7_DeviceNotAvailable"));
    ASSERT(0);
    return( 0 );
}

//=============================================================================
static int Int8_DoubleFault (TMeta * pMeta)
//=============================================================================
{
    TRACE(("Int8_DoubleFault"));
    ASSERT(0);
    return( 0 );
}

//=============================================================================
static int Int9_CoprocessorSegmentOverrun (TMeta * pMeta)
//=============================================================================
{
    TRACE(("Int9_CoprocessorSegmentOverrun"));
    ASSERT(0);
    return( 0 );
}

//=============================================================================
static int Int10_InvalidTSS (TMeta * pMeta)
//=============================================================================
{
    TRACE(("Int10_InvalidTSS"));
    ASSERT(0);
    return( 0 );
}

//=============================================================================
static int Int11_SegmentNotPresent (TMeta * pMeta)
//=============================================================================
{
    TRACE(("Int11_SegmentNotPresent"));
    ASSERT(0);
    return( 0 );
}

//=============================================================================
static int Int12_StackFault (TMeta * pMeta)
//=============================================================================
{
    TRACE(("Int12_StackFault"));
    ASSERT(0);
    return( 0 );
}

//=============================================================================
static int Int13_GeneralProtectionFault (TMeta * pMeta)
//=============================================================================
{
    BYTE *pTarget;
    DWORD dwVmLinear;

    TRACE(("Int13_GeneralProtectionFault"));

    // Almighty...General Protection Fault...
    ASSERT(0);

    switch( CPU.CPU_mode )
    {
        case CPU_REAL_MODE:
            ASSERT(0);
            break;

        case CPU_V86_MODE:
                // Reflect GPF down to the VM
                CPU_Exception( EXCEPTION_GPF, pMonitor->nErrorCode );

                // Switch to protected mode operations
                SetMainLoopState( stateNext, NULL );
                SetCpuMode( CPU_KERNEL_MODE );
            break;

        case CPU_KERNEL_MODE:
            ASSERT(0);
            break;
    }

    return( 0 );
}

//=============================================================================

extern void CpuPageFaultPagingOn();
extern void CpuPageFaultPagingOff();

static int Int14_PageFault (TMeta * pMeta)
//=============================================================================
{
    if( PAGING_ENABLED )
        CpuPageFaultPagingOn();
    else
        CpuPageFaultPagingOff();

    return( 0 );
}

//=============================================================================
static int Int16_FloatingPointError (TMeta * pMeta)
//=============================================================================
{
    TRACE(("Int16_FloatingPointError"));
    ASSERT(0);
    return( 0 );
}

//=============================================================================
static int Int17_AlignmentCheck (TMeta * pMeta)
//=============================================================================
{
    TRACE(("Int17_AlignmentCheck"));
    ASSERT(0);
    return( 0 );
}

//=============================================================================
static int Int18_MachineCheck (TMeta * pMeta)
//=============================================================================
{
    TRACE(("Int18_MachineCheck"));
    ASSERT(0);
    return( 0 );
}

