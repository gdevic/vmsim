/******************************************************************************
*                                                                             *
*   Module:     fn_iret.c													  *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       9/11/2000													  *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the code for the virtualization of the IRET/IRETD
		instruction.

    REAL-ADDRESS-MODE

    PROTECTED-MODE
        RETURN-FROM-V86-MODE
        RETURN-TO-V86-MODE
        TASK-RETURN
        PROTECTED-MODE-RETURN
            RETURN-TO-SAME-PRIVILEGE-LEVEL
            RETURN-TO-OUTER-PRIVILEGE-LEVEL

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 9/11/2000	 Original                                             Goran Devic *
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


/******************************************************************************
*                                                                             *
*   Local Defines, Variables and Macros                                       *
*                                                                             *
******************************************************************************/
#define RETURN_ADVANCE_EIP              0
#define RETURN_KEEP_EIP                 1


/******************************************************************************
*                                                                             *
*   Functions                                                                 *
*                                                                             *
******************************************************************************/

void fnCopyStackV86(void)
{
    ;
}


DWORD fn_iret(DWORD flags, BYTE bModrm)
{
    DWORD tempEIP, tempEFL, tempESP;
    WORD tempCS, tempSS;
    TGDT_Gate *pSel;

    switch( CPU.CPU_mode )
    {
        case CPU_REAL_MODE:             // REAL-ADDRESS-MODE
                if( flags & DIS_DATA32 )
                {
                    // Operand size is 32 bit
                    ASSERT(0);      // Not yet debugged - no code exercise it so far...

                    pMonitor->guestTSS.eip = CPU_PopDword();
                    pMonitor->guestTSS.cs  = LOWORD( CPU_PopDword() );
                    // "Intel Instruction Set Reference", 11-231
                    CPU.eflags = (CPU_PopDword() & 0x257FD5) | (CPU.eflags & 0x1A0000);
                    pMonitor->guestTSS.eflags = EFLAGS_SET_CONSTANTS(CPU.eflags & EFLAGS_ACTIVE_MASK) | VM_MASK | IF_MASK;
                }
                else
                {
                    // Operand size is 16 bit

                    pMonitor->guestTSS.eip = CPU_PopWord();
                    pMonitor->guestTSS.cs  = CPU_PopWord();
                    CPU.eflags = CPU_PopWord();
                    pMonitor->guestTSS.eflags = EFLAGS_SET_CONSTANTS(CPU.eflags & EFLAGS_ACTIVE_MASK) | VM_MASK | IF_MASK;
                }
            break;

        case CPU_V86_MODE:              // RETURN-FROM-V86-MODE
            ASSERT(0);
                ASSERT(CPU.cr0 & BITMASK(CR0_PE_BIT));      // Protected mode is on
                ASSERT(CPU.eflags & VM_MASK);               // Executing in V86 mode
                if( (CPU.eflags & IOPL_MASK) == IOPL_MASK ) // IOPL=3
                {
                    if( flags & DIS_DATA32 )
                    {
                        // Operand size is 32 bit

                        pMonitor->guestTSS.eip = CPU_PopDword();
                        pMonitor->guestTSS.cs  = LOWORD( CPU_PopDword() );
                        // "Intel Instruction Set Reference", 11-232
                        CPU.eflags = CPU_PopDword() & ~(VM_MASK | IOPL_MASK | VIP_MASK | VIF_MASK);
                        pMonitor->guestTSS.eflags = EFLAGS_SET_CONSTANTS(CPU.eflags & EFLAGS_ACTIVE_MASK) | VM_MASK | IF_MASK;
                    }
                    else
                    {
                        // Operand size is 16 bit

                        pMonitor->guestTSS.eip = CPU_PopWord();
                        pMonitor->guestTSS.cs  = CPU_PopWord();
                        CPU.eflags = CPU_PopWord() & ~(IOPL_MASK);
                        pMonitor->guestTSS.eflags = EFLAGS_SET_CONSTANTS(CPU.eflags & EFLAGS_ACTIVE_MASK) | VM_MASK | IF_MASK;
                    }
                }
                else
                {
                    // Trap to V86 monitor
                    CPU_Exception( EXCEPTION_GPF, 0 );
                }

            break;

        case CPU_KERNEL_MODE:
            ASSERT(0);
                if( CPU.eflags & NT_MASK )      // TASK-RETURN
                {   ;
                }
                else
                {
                    if( flags & DIS_DATA32 )
                    {
                        // Operand size is 32 bit

                        tempEIP = CPU_PopDword();
                        tempCS  = LOWORD( CPU_PopDword() );
                        tempEFL = CPU_PopDword();
                    }
                    else
                    {
                        // Operand size is 16 bit

                        tempEIP = CPU_PopWord();
                        tempCS  = LOWORD( CPU_PopWord() );
                        tempEFL = CPU_PopWord();
                    }
                    if( tempEFL & VM_MASK )
                    {
                        // RETURN-TO-V86-MODE
                        pMonitor->guestTSS.eip = tempEIP;
                        pMonitor->guestTSS.cs  = tempCS;
                        CPU.eflags = tempEFL;
                        pMonitor->guestTSS.eflags = EFLAGS_SET_CONSTANTS(CPU.eflags & EFLAGS_ACTIVE_MASK) | VM_MASK | IF_MASK;
                        tempESP = CPU_PopDword();
                        tempSS  = LOWORD( CPU_PopDword() );
                        pMonitor->guestTSS.es = LOWORD( CPU_PopDword() );
                        pMonitor->guestTSS.ds = LOWORD( CPU_PopDword() );
                        pMonitor->guestTSS.fs = LOWORD( CPU_PopDword() );
                        pMonitor->guestTSS.gs = LOWORD( CPU_PopDword() );
                        pMonitor->guestTSS.ss = tempSS;
                        pMonitor->guestTSS.esp = tempESP;

                        SetCpuMode( CPU_V86_MODE );        // Continue execution in V86 mode
                        SetMainLoopState( stateNative, fnCopyStackV86 );
                    }
                    else
                    {
                        // PROTECTED-MODE-RETURN
                        if( tempCS != 0 )
                        {
                            if( tempCS < CPU.guestGDT.limit )
                            {
                                pSel = &pMonitor->GDT[ tempCS >> 3];
                                if( (pSel->type & 8) == 8 )   // Get only CODE bit of a type field
                                {
                                    if( pSel->present == TRUE )
                                    {
                                        if( pSel->dpl > VM_CPL )
                                        {
                                            // RETURN-TO-OUTER-PRIVILEGE-LEVEL
                                            ASSERT(0);
                                        }
                                        else
                                        {
                                            // RETURN-TO-SAME-PRIVILEGE-LEVEL
                                            if( tempEIP < GetGDTLimit(pSel) )
                                            {
                                                pMonitor->guestTSS.eip = tempEIP;
                                                pMonitor->guestTSS.cs  = tempCS | SEL_RPL1_MASK;
                                                if( flags & DIS_DATA32 )
                                                    CPU.eflags = (CPU.eflags & ~(RF_MASK | AC_MASK | ID_MASK)) | (tempEFL & (RF_MASK | AC_MASK | ID_MASK));
                                                if( VM_CPL <= ((CPU.eflags & IOPL_MASK) >> IOPL_BIT0) )
                                                    CPU.eflags = (CPU.eflags & ~IF_MASK) | (tempEFL & IF_MASK);
                                                // TODO: This is messy - 16 bit pmode code will probably not work on this...
                                                if( PERCIEVED_CPL==0 )
                                                    CPU.eflags = tempEFL;
                                            }
                                            else
                                                CPU_Exception( EXCEPTION_GPF, 0 );
                                        }
                                    }
                                    else
                                        CPU_Exception( EXCEPTION_SEG_NOT_PRESENT, tempCS & 0xFFFC );
                                }
                                else
                                {
                                    ASSERT(0);
                                    CPU_Exception( EXCEPTION_GPF, tempCS & 0xFFFC );
                                }
                            }
                            else
                                CPU_Exception( EXCEPTION_GPF, tempCS & 0xFFFC );
                        }
                        else
                            CPU_Exception( EXCEPTION_GPF, 0 );
                    }
                }
            break;
    }

    return( RETURN_KEEP_EIP );
}

