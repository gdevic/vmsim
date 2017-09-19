/******************************************************************************
*                                                                             *
*   Module:     Scanner.c                                                     *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       4/5/2000                                                      *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the Scan-Before-Execute code.

    Meta-page contains clue-bytes mapped to the equivalent offsets within
    a page to the corresponding code page.

    The format of a meta-byte is:
    ÚÄÄÄÂÄÄÄÂÄÄÄÂÄÄÄÂÄÄÄÂÄÄÄÂÄÄÄÂÄÄ¿
    ³ 7 ³ 6 ³ 5 ³ 4 ³ 3 ³ 2 ³ 1 ³ 0³
    ÀÄÄÄÁÄÄÄÁÄÄÄÁÄÄÄÁÄÄÄÁÄÄÄÁÄÄÄÁÄÄÙ
              |   |   |   |   |   +--+  Instruction len in bytes [1,15]
              |   |   |   |   +------|
              |   |   |   +----------|
              |   |   +--------------+
              |   |
              |   +------------------+  Instruction behaviour:
              +----------------------+     0 - normal instruction
                                           1 - cross-page: do single step
                                           2 - terminating
                                           3 - illegal instruction

    Whole byte=0 means opcode not yet scanned.

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 4/5/2000   Original                                             Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Include Files                                                             *
******************************************************************************/
#include <strmini.h>                // Include driver level C functions

#include <ntddk.h>                  // Include ddk function header file

#include "Vm.h"                     // Include root header file

#include "Scanner.h"                // Include scanner defines

#include "DisassemblerInterface.h"

/******************************************************************************
*                                                                             *
*   Global Variables                                                          *
*                                                                             *
******************************************************************************/

TDisassembler Dis;
TDisassembler *pDis = &Dis;

/******************************************************************************
*                                                                             *
*   Local Defines, Variables and Macros                                       *
*                                                                             *
******************************************************************************/

static DWORD dwMetaPageBase;
static DWORD dwCSIPPageBase;
static BYTE  szDisasm[256];

/******************************************************************************
*                                                                             *
*   Functions                                                                 *
*                                                                             *
******************************************************************************/

extern int ScanOneInstruction();
static void ScanOneLeg( BYTE *pCSIP );


/******************************************************************************
*                                                                             *
*   DWORD GetDisFlags(void)                                                   *
*                                                                             *
*******************************************************************************
*
*   Returns the flags containing the data/address mode of operation.
*
*   Returns:
*       DIS_DATA32 | DIS_ADDRESS32
*
******************************************************************************/
DWORD GetDisFlags(void)
{
    // Determine the current code and data bit-ness: it is 16 bit unless
    // the mode is PM and cs selector is 32-bit

    if( !(pMonitor->guestTSS.eflags & VM_MASK)
        && pMonitor->GDT[ pMonitor->guestTSS.cs>>3 ].size32 )
        return(DIS_ADDRESS32 | DIS_DATA32);

    return(0);                  // 16 bit code and data
}


//
// Helper function: Disassemble single line that CS:EIP points at
//
void DisassembleCSIP()
{
    Dis.bpEffTarget = (BYTE *)(pMonitor->guestTSS.cs << 4) + pMonitor->guestTSS.eip;
    Dis.bpTarget    = Mem.MemVM.linear + Dis.bpEffTarget;
    Dis.szDisasm    = szDisasm;
    Dis.dwFlags     = GetDisFlags();
    Disassembler( &Dis );

    DbgPrint(("%04X:%04X   ", pMonitor->guestTSS.cs, pMonitor->guestTSS.eip));
    Dis.nScanEnum = Dis.bInstrLen;
    for( ; Dis.bInstrLen>0 ;Dis.bInstrLen-- )
        DbgPrint((" %02X", *(BYTE*)Dis.bpTarget++ ));
    for( ;Dis.nScanEnum<6 ;Dis.nScanEnum++ )
        DbgPrint(("   "));
    DbgPrint(("  %s\n", Dis.szDisasm ));
}


/******************************************************************************
*                                                                             *
*   int ScanCodePage( BYTE *pCSIP, BYTE *pMetaPage, DWORD Mode )              *
*                                                                             *
*******************************************************************************

    This function scans the currently-executing code page starting at the
    given linear location.

    This scanner will scan until a terminating condition is reached:
        1) current instruction crossed a page boundary
        2) terminal instruction is reached (one that needs virtualization)
        3) current instruction is already scanned
        4) invalid opcode

    As additional performance improvement, certain decoded instructions will
    cause the following to happen:
        Short displacement jump on condition 0x70-0x7F
            (always a byte displacement)
         -> scan recursively both paths until a terminal condition
        Long displacement jump on condition 0x0F & 0x80-0x8F
            (word or doubleword displacement)
         -> scan recursively both paths until a terminal condition
        LOOPxx instructions 0xE0-0xE3
            (always a byte displacement)
         -> scan recursively both paths until a terminal condition
        CALL relative offset 0xE8
            (word or doubleword displacement)
         -> scan recursively both paths until a terminal condition
        JMP relative offset 0xE9
            (word or doubleword displacement)
         -> start scanning from the new offset until a terminal condition
        JMP relative offset 0xEB
            (always a byte displacement)
         -> start scanning from the new offset until a terminal condition

*******************************************************************************
*   Where:
*       pCSIP is the starting address to scan
*       pMetaPage is the base pointer to a meta page
*       dwMode may contain:
*               bit 0 - when set, 32 bit data
*               bit 1 - when set, 32 bit code
*
*   Returns:
*       0
*
******************************************************************************/

BYTE *baseCSIP;
WORD xxcs = 0;
DWORD xxeip = 0xFFFFFFFF;
DWORD delta;

int ScanCodePage( BYTE *pCSIP, BYTE *pMetaPage, DWORD Mode )
{
    ASSERT(((int)pMetaPage & 0xFFF) == 0);

    dwMetaPageBase = (int)pMetaPage;
    dwCSIPPageBase = (int)pCSIP & ~0xFFF;

#if DBG
    if( fSingleStep )
    {
        // Disassemble single line that CS:EIP points at

        Dis.bpTarget    = pCSIP;
        Dis.bpEffTarget = (BYTE *)(pMonitor->guestTSS.cs << 4) + pMonitor->guestTSS.eip;
        Dis.szDisasm    = szDisasm;
        Dis.dwFlags     = GetDisFlags();
        Disassembler( &Dis );

        DbgPrint(("%04X:%04X   ", pMonitor->guestTSS.cs, pMonitor->guestTSS.eip));
        DbgPrint(("  %s\n", Dis.szDisasm ));
    }
    baseCSIP = pCSIP;
#endif

    ScanOneLeg( pCSIP );

    return( 0 );
}

static int stopdisat=0;

static void ScanOneLeg( BYTE *pCSIP )
{
    BYTE * pMetaPage = (BYTE *) (dwMetaPageBase + ((DWORD)pCSIP & 0xFFF));
    int nLen;

    while(1)
    {
        // Make sure the instruction is not already scanned

        if( *pMetaPage != 0 )
        {
            return;
        }

#if DBG //=======================================================================

if( stopdisat==(int)pCSIP ) Breakpoint;
delta = (DWORD) (pCSIP - baseCSIP);
if( (xxcs==pMonitor->guestTSS.cs) && (xxeip==pMonitor->guestTSS.eip+delta) )
    _asm int 3;
if( 0 && (pMonitor->guestTSS.cs==0xC000 || pMonitor->guestTSS.cs==0xF000) ) goto skipit;
if( !fSingleStep )
{
    // Disassemble instruction and print it to a debug out device
if( !fSilent )
{
    Dis.bpTarget    = pCSIP;
    Dis.bpEffTarget = (BYTE*) CSIPtoVmLinear();
    Dis.szDisasm    = szDisasm;
    Dis.dwFlags     = GetDisFlags();
    Disassembler( &Dis );

    DbgPrint(("%04X:%04X   ", pMonitor->guestTSS.cs, pMonitor->guestTSS.eip + delta));
//    DbgPrint(("%08X %02X: ", pCSIP, Dis.bInstrLen));
    Dis.nScanEnum = Dis.bInstrLen;
    for( ; Dis.bInstrLen>0 ;Dis.bInstrLen-- )
        DbgPrint((" %02X", *(BYTE*)Dis.bpTarget++ ));
    for( ;Dis.nScanEnum<6 ;Dis.nScanEnum++ )
        DbgPrint(("   "));
    DbgPrint(("  %s\n", Dis.szDisasm ));
}
}
skipit:
#endif  //=======================================================================

        // Now for real... Use cut version of disassembler to get the instr. stats
        // We either scan pages for 16 bit real/v86 mode, or 16/32 bit PM

        Dis.dwFlags = GetDisFlags();
        Dis.bpTarget = pCSIP;

        nLen = ScanOneInstruction();

        ASSERT(nLen < 16);

        // Is the instruction legal ?
        // "If not, I will make it legal." -- Senator Palpatine, The Phantom Menace

        if( nLen==0 )
        {
            // Illegal instruction, set the undefined instrution meta-byte

            ASSERT(0);      // For now scream if found illigal instruction !  Good error catcher!

            *pMetaPage = (META_UNDEF << 4) + nLen;
            *pCSIP     = INT3;

            return;
        }

        if( ((DWORD)(pCSIP + nLen) & ~0xFFF) != dwCSIPPageBase )
        {
            // Current instruction spans out of this page...

            if( Dis.nScanEnum==SCAN_TERMINATING )
            {
                // Instruction was terminating as well

                *pMetaPage = (META_TERMINATING << 4) + nLen;
                *pCSIP     = INT3;

                return;
            }
            else
            {
                // Single step over it to bring us to a new page

                *pMetaPage = (META_SINGLE_STEP << 4) + nLen;
                *pCSIP     = INT3;

                return;
            }
        }

        // If the instruction can be optimized, try to do it

        switch( Dis.nScanEnum )
        {
            case SCAN_NATIVE:  // Normal instruction
                    // Set the instruction meta byte to execute it natively

                    *pMetaPage = (BYTE) nLen;

                break;

            case SCAN_COND_JUMP:    // See if we can evaluate both paths
#if DBG
                    if( 0 )
#else
                    if( ((DWORD)(pCSIP + nLen + Dis.nDisplacement) & ~0xFFF)==dwCSIPPageBase )
#endif
                    {
                        // OK for a conditional jump or a call..

                        *pMetaPage = (BYTE) nLen;

                        // ..it is on the same page; call a branch recursively...

                        ScanOneLeg( pCSIP + nLen + Dis.nDisplacement );

                        // ...And proceed with the next instruction
                    }
                    else
                    {   // Conditional jump is out-of-page, so we need to stop on it
                        // Single-step will give us a chance to follow it

                        *pMetaPage = (META_SINGLE_STEP << 4) + nLen;
                        *pCSIP     = INT3;

                        return;
                    }
                break;

            case SCAN_JUMP:     // Simple jump to a new offset.. Check the new address

#if DBG
                    if( 0 )
#else
                    if( ((DWORD)(pCSIP + nLen + Dis.nDisplacement) & ~0xFFF)==dwCSIPPageBase )
#endif
                    {
                        // OK to jump... we are on the same page
                        // Set the instruction meta byte to execute it natively

                        *pMetaPage = (BYTE) nLen;

                        // Move on to the next instruction (nLen will be added below)

                        pCSIP     += Dis.nDisplacement;
                        pMetaPage += Dis.nDisplacement;

                        break;
                    }
                    else
                    {   // Nope! This jump is out-of-page
                        // Single-step will give us a chance to follow it

                        *pMetaPage = (META_SINGLE_STEP << 4) + nLen;
                        *pCSIP     = INT3;

                        return;
                    }

            case SCAN_TERMINATING:
                    // Nothing we can do here.. it's a terminating instruction

                    *pMetaPage = (META_TERMINATING << 4) + nLen;
                    *pCSIP     = INT3;

                    return;

            case SCAN_TERM_PMODE:
                    // These instructions are terminating only in protected mode

                    if( CPU.CPU_mode == CPU_KERNEL_MODE )
                    {
                        *pMetaPage = (META_TERMINATING << 4) + nLen;
                        *pCSIP     = INT3;

                        return;
                    }

                    // In V86 and 'Real' mode it is executed natively

                    *pMetaPage = (BYTE) nLen;

                break;

            case SCAN_SINGLE_STEP:  // Single step over this instruction
                    // Single-step will give us a chance to follow it wherever it goes

                    *pMetaPage = (META_SINGLE_STEP << 4) + nLen;
                    *pCSIP     = INT3;

                    return;

            default:    // We should not be here
                    ASSERT(0);
                break;
        }

        pCSIP     += nLen;
        pMetaPage += nLen;
    }
}
