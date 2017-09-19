/******************************************************************************
*                                                                             *
*   Module:     DisassemblerForVirtualization.c                               *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       4/28/2000                                                     *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

    This file contains the basic disassembler needed for virtualization.

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
#include <strmini.h>                // Include wdm support

#include "Vm.h"                     // Include root header file

#include "DisassemblerInterface.h"

#include "DisassemblerDefines.h"    // Include disassembler tools

/******************************************************************************
*                                                                             *
*   Global Variables                                                          *
*                                                                             *
******************************************************************************/

typedef DWORD (*TFaulting)(DWORD flags, BYTE bModrm, DWORD address, TMEM *pMem);
typedef DWORD (*TProtected)(DWORD flags, BYTE bModrm);

extern TFaulting Faulting[];
extern TProtected Protected[];

DWORD CurrentInstrLen;                  // Current instruction lenght in bytes

/******************************************************************************
*                                                                             *
*   External functions (optional)                                             *
*                                                                             *
******************************************************************************/

/******************************************************************************
*                                                                             *
*   Local Defines, Variables and Macros                                       *
*                                                                             *
******************************************************************************/

static BYTE    *bpTarget;           // Pointer to the target code to be disassembled
static DWORD    dwVmCSIP;           // Target code in VM address space

#define NEXTBYTE    VmGetNextBYTE()
#define NEXTWORD    VmGetNextWORD()
#define NEXTDWORD   VmGetNextDWORD()

// These byte, word and dword fetch macros are now functions that are aware
// of the VM address space and will start fetching from the new appropriate page
// if a page boundary is reached

BYTE VmGetNextBYTE()
{
    BYTE bValue;

    bValue = *bpTarget;
    bpTarget  += 1;
    CurrentInstrLen += 1;

    if( ((int)bpTarget & 0xFFF) == 0x000 )
    {
        if((bpTarget = (BYTE*)VmLinearToDriverLinear(dwVmCSIP+CurrentInstrLen))==NULL)
            ASSERT(0);
    }

    return( bValue );
}

WORD VmGetNextWORD()
{
    WORD wValue;

    CurrentInstrLen += 2;

    if( ((int)bpTarget & 0xFFF) < 0xFFE )
    {
        wValue = *(WORD *) bpTarget;
        bpTarget  += 2;
    }
    else
    {
        switch( ((int)bpTarget & 0xFFF) )
        {
            case 0xFFE:     // We can read, but we need a new address for later
                    wValue = *(WORD *) bpTarget;
                    if((bpTarget = (BYTE*)VmLinearToDriverLinear(dwVmCSIP+CurrentInstrLen))==NULL)
                        ASSERT(0);
                break;

            case 0xFFF:     // We can read one byte from each page
                    wValue = (WORD) *bpTarget;
                    if((bpTarget = (BYTE*)VmLinearToDriverLinear(dwVmCSIP+CurrentInstrLen))==NULL)
                        ASSERT(0);
                    wValue += (*(bpTarget-1)) << 8;
                break;
        }
    }

    return( wValue );
}

DWORD VmGetNextDWORD()
{
    DWORD dwValue;

    CurrentInstrLen += 4;

    if( ((int)bpTarget & 0xFFF) < 0xFFC )
    {
        dwValue = *(DWORD *) bpTarget;
        bpTarget  += 4;
    }
    else
    {
        switch( ((int)bpTarget & 0xFFF) )
        {
            case 0xFFC:     // We can read, but we need a new address for later
                    dwValue = *(DWORD *) bpTarget;
                    CurrentInstrLen += 4;
                    if((bpTarget = (BYTE *)VmLinearToDriverLinear(dwVmCSIP+CurrentInstrLen))==NULL )
                        ASSERT(0);
                break;

            case 0xFFD:     // We can read one byte before needing a new page
                    dwValue  = (DWORD) *(BYTE *)bpTarget;
                    if((bpTarget = (BYTE*)VmLinearToDriverLinear(dwVmCSIP+CurrentInstrLen))==NULL )
                        ASSERT(0);
                    dwValue += (DWORD) (*(DWORD *)(bpTarget-3)) << 8;
                break;

            case 0xFFE:     // We can read two bytes before needing a new page
                    dwValue = (DWORD) *(WORD *)bpTarget;
                    if((bpTarget = (BYTE*)VmLinearToDriverLinear(dwVmCSIP+CurrentInstrLen))==NULL )
                        ASSERT(0);
                    dwValue += (*(WORD *)(bpTarget-2)) << 16;
                break;

            case 0xFFF:     // We can read three bytes before needing a new page
                    dwValue  = (DWORD) *(BYTE *)bpTarget;
                    dwValue += (DWORD) (*(WORD *)(bpTarget+1)) << 8;
                    if((bpTarget = (BYTE*)VmLinearToDriverLinear(dwVmCSIP+CurrentInstrLen))==NULL )
                        ASSERT(0);
                    dwValue += (DWORD) (*(BYTE *)(bpTarget-1)) << 24;
                break;
        }
    }

    return( dwValue );
}

/******************************************************************************
*                                                                             *
*   Functions                                                                 *
*                                                                             *
******************************************************************************/

/******************************************************************************
*                                                                             *
*   int DisVirtualize( BYTE *bpMyTarget, DWORD VmLinear, DWORD dwFlags )      *
*                                                                             *
*******************************************************************************
*
*   This functions quickly decodes instruction and calls its virtualization
*   code.
*
*   Where:
*       bpTarget is the linear address of the instruction in driver space
*       VmLinear is the linear address of the instruction in Vm address space
*       pMem is either NULL (for virtualization of protected instructions), or
*           a pointer to memory descriptor of a registered faulting region
*           (for virtualization of memory access faulting instructions)
*
*   Returns:
*       1 - instruction needs to be single-stepped over (don't advance EIP)
*       0 - instruction was nicely virtualized, so advance eip on return
*
*   Warning: Since we are doing only partial disassembly, you can't rely on
*            the instruction length being large enough for instructions
*            that are not simple to decode.
*
******************************************************************************/
int DisVirtualize( BYTE *bpMyTarget, DWORD VmLinear, TMEM *pMem  )
{
    TOpcodeData *p;             // Pointer to a current instruction record
    DWORD   dwSegment;          // Effective segment register
    DWORD   dwFlags;            // default operand and address bits + extra
    BYTE    bOpcode;            // Current opcode that is being disassembled
    BYTE    bReg;               // Register field of the instruction
    BYTE    data_len;

    CurrentInstrLen = 0;        // Reset instruction len to 0
    dwVmCSIP  = VmLinear;       // Set the starting Vm linear address
    bpTarget  = bpMyTarget;     // Set the global target address

    dwSegment = _S_DS - _S_ES;  // Set default segment of DS

    dwFlags = GetDisFlags();

    do
    {
        bOpcode = NEXTBYTE;     // Get the first opcode byte from the target address
        p = &Op1[bOpcode];      // Get the address of the instruction record

        if( p->flags & DIS_SPECIAL )
        {
            // Opcode is one of the special ones, so do what needs to be done there

            switch( p->name )
            {
                case _EscD8:
                case _EscD9:
                case _EscDA:
                case _EscDB:
                case _EscDC:
                case _EscDD:
                case _EscDE:
                case _EscDF:        // Coprocessor escape: bytes D8 - DF
                    bOpcode = NEXTBYTE;             // Get the modRM byte of the instruction

                    if( bOpcode < 0xC0 )
                    {
                        // Opcodes 00-BF use Coproc1 table

                        bReg = (bOpcode >> 3) & 7;
                        p = &Coproc1[ p->name - _EscD8 ][ bReg ];

                        goto StartInstructionParseMODRM;
                    }
                    // Opcodes C0-FF use Coproc2 table

                    p = &Coproc2[ p->name - _EscD8 ][ bOpcode - 0xC0 ];

                goto StartInstructionNoMODRM;

                case _S_ES:         // Segment override:
                case _S_CS:
                case _S_SS:
                case _S_DS:         // Set new segment (0 - 5)
                case _S_FS:         // and signal that it is overriden from default
                case _S_GS:
                    dwSegment = p->name - _S_ES;
                    dwFlags  |= DIS_SEGOVERRIDE;
                continue;

                case _OPSIZ:        // Operand size override - toggle
                    dwFlags ^= DIS_DATA32;
                continue;

                case _ADSIZ:        // Address size override - toggle
                    dwFlags ^= DIS_ADDRESS32;
                continue;

                case _REPNE:        // REPNE/REPNZ prefix
                    dwFlags |= DIS_REPNE;
                continue;

                case _REP:          // REP/REPE/REPZ prefix
                    dwFlags |= DIS_REP;
                continue;

                case _2BESC:        // 2 byte escape code 0x0F
                    bOpcode = NEXTBYTE;             // Get the second byte of the instruction
                    p = &Op2[bOpcode];              // Get the address of the instruction record

                    if( !(p->flags & DIS_SPECIAL) ) goto StartInstruction;
                    if( p->name < _GRP6 ) goto IllegalOpcode;

                case _GRP1a:        // Additional groups of instructions
                case _GRP1b:
                case _GRP1c:
                case _GRP2a:
                case _GRP2b:
                case _GRP2c:
                case _GRP2d:
                case _GRP2e:
                case _GRP2f:
                case _GRP3a:
                case _GRP3b:
                case _GRP4:
                case _GRP5:
                case _GRP6:
                case _GRP7:
                case _GRP8:
                case _GRP9:

                    bOpcode = NEXTBYTE;             // Get the Mod R/M byte whose...
                                                    // bits 3,4,5 select instruction

                    bReg = (bOpcode >> 3) & 7;
                    p = &Groups[p->name - _GRP1a][ bReg ];

                    if( !(p->flags & DIS_SPECIAL) ) goto StartInstructionParseMODRM;

                case _NDEF :        // Not defined or illegal opcode
                    goto IllegalOpcode;

                default :;          // Should not happen
            }
        }
        else
            goto StartInstruction;
    }
    while( CurrentInstrLen < 15 );

IllegalOpcode:

    ASSERT(0);

    return( 0 );

StartInstruction:

    // If this instruction needs additional Mod R/M byte, fetch it

    if( p->flags & DIS_MODRM )
    {
        // Get the next byte (modR/M bit field)
        bOpcode = NEXTBYTE;
    }

StartInstructionParseMODRM:
StartInstructionNoMODRM:

    // Call the virtual instruction and return its value

    if( pMem != NULL )
    {
        ////////////////////////////////////////////////////////////////////////
        // We came from the page fault handler, where the instruction accessed
        // reserved memory regions
        ////////////////////////////////////////////////////////////////////////
        // There are two mechanisms to deal with those instrcutions:
        // 1. Simulate instruction in software
        //    This we can do for speed, as example `rep stos' are written in
        //    software as repeatingly switching to a VM would kill the performance
        // 2. Using a `legacy' paging technique and letting the instruction
        //    execute with the temp pages being set up

        if( p->access != 0 )
        {
            // Faulting instruction will be handled by the legacy paging method

            // If the data size depends on the operand size, adjust it here
            if( (data_len = (p->access & 0xF)) == INSTR_WORD_DWORD )
            {
                if( dwFlags & DIS_DATA32 )
                    data_len = INSTR_DWORD;
                else
                    data_len = INSTR_WORD;
            }

            // Since all memory access instructions need post-processing,
            // set single step and do not advance eip

            SetMainLoopState( stateStep, MemoryAccess(p->access, data_len, pMem) );
            return( 1 );
        }
        else
        {
            if( (p->v_instruction >= 0x80) && (p->v_instruction <= F_LAST) )
            {
                // Faulting instruction will be fully simulated in software

                return (*Faulting[ p->v_instruction & 0x7F ])(dwFlags | (dwSegment << 16), bOpcode, pMonitor->guestCR2, pMem );
            }
            else    // Error catcher for instructions that are not (yet) handled
            {
                ERROR(("Memory access instruction not handled: %s", sNames[p->name] ));
                DisassembleCSIP();
                ASSERT(0);
            }
        }
    }
    else
    {
        ////////////////////////////////////////////////////////////////////////
        // We came due to our prescan-placed breakpoint and those instructions
        // are terminating in need of virtualization
        ////////////////////////////////////////////////////////////////////////

        ASSERT(p->v_instruction <= V_LAST);
        return (*Protected[ p->v_instruction ])(dwFlags | (dwSegment << 16), bOpcode );
    }

    ASSERT(0);      // Why we are here ????
    return( 0 );
}


//=============================================================================
// This heler function decodes memory offset and returns it
//=============================================================================
static DWORD DecodeOffset( DWORD *pFlags, BYTE bOpcode )
{
    DWORD   dwDword;            // Returning value
    BYTE    bReg;               // Register field of the Mod/RM byte
    BYTE    bMod;               // Mod field of the Mod/RM byte
    BYTE    bRm;                // R/M field of the Mod/RM byte

    BYTE    bSib;               // S-I-B byte for the instruction
    BYTE    bSs;                // SS field of the s-i-b byte
    BYTE    bIndex;             // Index field of the s-i-b byte
    BYTE    bBase;              // Base field of the s-i-b byte

    bReg = (bOpcode >> 3) & 7;
    bMod = bOpcode >> 6;
    bRm  = bOpcode & 7;

    dwDword = 0;

    if( *pFlags & DIS_ADDRESS32 )
    {
        // 32 bit addressing modes
        // These modes may have an additional SIB byte immediately folowing

        if( (bMod != 3) && (bRm==4) )
        {
            // Get the s-i-b byte and parse it

            bSib = NEXTBYTE;

            bSs = bSib >> 6;
            bIndex = (bSib >> 3) & 7;
            bBase = bSib & 7;

            // Special case when base=5, disp32 with no base if mod is 00

            if( (bBase==5) && (bMod==0) )
            {
                dwDword = NEXTDWORD;
            }
            else
            {
                // General 32 bit register value serves as base, indexed by bBase

                dwDword = *(((DWORD *)&pMonitor->guestTSS.eax) + bBase );
            }

            // Scaled index by bSs scale factor, (no index if bIndex is 4)

            if( bIndex != 4 )
            {
                dwDword += (*(((DWORD *)&pMonitor->guestTSS.eax) + bIndex )) << bSs;
            }
        }

        switch( bMod )
        {
            case 0:         // Complex memmory location, possible segment override
                switch( bRm )
                {
                    case 0: dwDword += pMonitor->guestTSS.eax;  break;
                    case 1: dwDword += pMonitor->guestTSS.ecx;  break;
                    case 2: dwDword += pMonitor->guestTSS.edx;  break;
                    case 3: dwDword += pMonitor->guestTSS.ebx;  break;
                    case 4: ;  break;
                    case 5: dwDword = NEXTDWORD;  break;
                    case 6: dwDword += pMonitor->guestTSS.esi;  break;
                    case 7: dwDword += pMonitor->guestTSS.edi;  break;
                }
            break;

            case 1:         // 8 bit displacement added to a register and index

                dwDword += (DWORD)(signed) NEXTBYTE;

                // Add index register

                dwDword += *(((DWORD *)&pMonitor->guestTSS.eax) + bReg );
            break;

            case 2:         // 16 bit displacement added to a register and index

                dwDword += (DWORD) NEXTWORD;

                // Add index register

                dwDword += *(((DWORD *)&pMonitor->guestTSS.eax) + bReg );
            break;

            case 3:         // Register not allowed with memory pointers
                ASSERT(0);
            break;
        }
    }
    else  //==========================================================================
    {
        // 16 bit addressing modes

        switch( bMod )
        {
            case 0:
                switch( bRm )
                {
                    case 0: dwDword = LOWORD(pMonitor->guestTSS.ebx) + LOWORD(pMonitor->guestTSS.esi);  break;
                    case 1: dwDword = LOWORD(pMonitor->guestTSS.ebx) + LOWORD(pMonitor->guestTSS.edi);  break;
                    case 2: dwDword = LOWORD(pMonitor->guestTSS.ebp) + LOWORD(pMonitor->guestTSS.esi);  break;
                    case 3: dwDword = LOWORD(pMonitor->guestTSS.ebp) + LOWORD(pMonitor->guestTSS.edi);  break;
                    case 4: dwDword = LOWORD(pMonitor->guestTSS.esi);  break;
                    case 5: dwDword = LOWORD(pMonitor->guestTSS.edi);  break;
                    case 6: dwDword = (DWORD) NEXTWORD; break;
                    case 7: dwDword = LOWORD(pMonitor->guestTSS.ebx);  break;
                }
            break;

            case 1:
                switch( bRm )
                {
                    case 0: dwDword = LOWORD(pMonitor->guestTSS.ebx) + LOWORD(pMonitor->guestTSS.esi);  break;
                    case 1: dwDword = LOWORD(pMonitor->guestTSS.ebx) + LOWORD(pMonitor->guestTSS.edi);  break;
                    case 2: dwDword = LOWORD(pMonitor->guestTSS.ebp) + LOWORD(pMonitor->guestTSS.esi);  break;
                    case 3: dwDword = LOWORD(pMonitor->guestTSS.ebp) + LOWORD(pMonitor->guestTSS.edi);  break;
                    case 4: dwDword = LOWORD(pMonitor->guestTSS.esi);  break;
                    case 5: dwDword = LOWORD(pMonitor->guestTSS.edi);  break;
                    case 6: dwDword = LOWORD(pMonitor->guestTSS.ebp);  break;
                    case 7: dwDword = LOWORD(pMonitor->guestTSS.ebx);  break;
                }
                // Add sign-extended 8 bit displacement

                dwDword += (signed) NEXTBYTE;
            break;

            case 2:
                switch( bRm )
                {
                    case 0: dwDword = LOWORD(pMonitor->guestTSS.ebx) + LOWORD(pMonitor->guestTSS.esi);  break;
                    case 1: dwDword = LOWORD(pMonitor->guestTSS.ebx) + LOWORD(pMonitor->guestTSS.edi);  break;
                    case 2: dwDword = LOWORD(pMonitor->guestTSS.ebp) + LOWORD(pMonitor->guestTSS.esi);  break;
                    case 3: dwDword = LOWORD(pMonitor->guestTSS.ebp) + LOWORD(pMonitor->guestTSS.edi);  break;
                    case 4: dwDword = LOWORD(pMonitor->guestTSS.esi);  break;
                    case 5: dwDword = LOWORD(pMonitor->guestTSS.edi);  break;
                    case 6: dwDword = LOWORD(pMonitor->guestTSS.ebp);  break;
                    case 7: dwDword = LOWORD(pMonitor->guestTSS.ebx);  break;
                }
                // Add sign-extended 16 bit displacement

                dwDword += (signed) NEXTWORD;
            break;

            case 3:         // Register not allowed with memory pointers
                ASSERT(0);
            break;
        }
    }

    return( dwDword );
}


//=============================================================================
// This heler function forms the complete address given the segment/selector
// and offset values
//=============================================================================
static DWORD FormAddress( WORD wSel, DWORD dwOffset )
{
    DWORD dwDword;

    // wSel either represent the segment or selector, depending on the execution mode

    if( CPU.CPU_mode == CPU_KERNEL_MODE )
    {
        // CS contains selector into a descriptor table: it can be GDT or LDT

        if( wSel & SEL_TI_MASK )
        {
            // Local descriptor table
            ASSERT(0);
//            ASSERT(CPU.pLDT);
//            dwDword = GetGDTBase(&CPU.pLDT[ wSel >> 3 ]);
        }
        else
        {
            // Global descriptor table
            // TODO: Check limit against the offset

            ASSERT(CPU.pGDT);
            dwDword = GetGDTBase(&CPU.pGDT[ wSel >> 3 ]) + dwOffset;
        }
    }
    else
    {
        // wSel contains segment value
        // Running either `real` mode or v86 mode

        ASSERT(dwOffset <= 0xFFFF);
        dwDword = (wSel << 4) + dwOffset;
    }

    return( dwDword );
}


/******************************************************************************
*                                                                             *
*   DWORD DecodeMemoryPointer( DWORD *pFlags, BYTE bOpcode )                  *
*                                                                             *
*******************************************************************************
*
*   Decodes a memory pointer.  Registers are not allowed.  This function uses
*   instruction fetch macros NEXTBYTE, NEXTWORD and NEXTDWORD to fetch actual
*   bytes of instruction to decode.
*
*   Where:
*       pFlags is the address of flags bitfield.  Top 16 bit contain
*              selector index.
*       bOpcode is the instruction opcode.
*
*   Returns:
*       Effective pointer to memory where the instruction addresses to.
*
******************************************************************************/
DWORD DecodeMemoryPointer( DWORD *pFlags, BYTE bOpcode )
{
    DWORD   dwDword;            // Returning value
    DWORD   dwOffset;           // Operand offset
    WORD    wSel;               // Selector that is used to address

    // Decode base address (offset)

    dwOffset = DecodeOffset( pFlags, bOpcode );

    // Add the selector value to the effective address

    // TODO:
    // Also, some indices use SS as default segment, not DS (namely all that are using ebp register)
    // use DIS_SEGOVERRIDE flag to adjust

    wSel = LOWORD( *((DWORD *)&pMonitor->guestTSS.es + (*pFlags >> 16)) );

    dwDword = FormAddress( wSel, dwOffset );

    return( dwDword );
}


// Decodes Ew-type argument, that is (always word):
//  * from a memory location
//  * from a register

WORD DecodeEw( DWORD *pFlags, BYTE bOpcode )
{
    DWORD   dwDword;
    WORD   *pWord;              // Poiner to a word data
    BYTE    bMod;               // Mod field of the Mod/RM byte
    BYTE    bRm;                // R/M field of the Mod/RM byte

    bMod = bOpcode >> 6;
    bRm  = bOpcode & 7;

    // Quickly decode simple register

    if( bMod==3 )
    {
        dwDword = *(((DWORD *)&pMonitor->guestTSS.eax) + bRm );

        return( LOWORD(dwDword) );
    }

    // For more complicated address calculation call the pointer routine

    dwDword = DecodeMemoryPointer( pFlags, bOpcode );

    // Fetch a word from the given memory location

    if((pWord = (WORD *)VmLinearToDriverLinear(dwDword))==NULL )
        ASSERT(0);

    return( *pWord );
}

