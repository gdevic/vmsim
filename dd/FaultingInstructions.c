/******************************************************************************
*                                                                             *
*   Module:     FaultingInstructions.c                                        *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       5/16/2000                                                     *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

    Instructions that access memory that is reserved for an external virtual
    device are virtualized here.

    These virtual instructions are called from DisassemblerForVirtualization.c

    They should return RETURN_ADVANCE_EIP if the eip should be advanced,
    or RETURN_KEEP_EIP if the eip should not be advanced.
    (as example, when virtualized instruction explicitly changes eip).

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 5/16/2000  Original                                             Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Include Files                                                             *
******************************************************************************/
#include <strmini.h>                // Include driver level C functions

#include <ntddk.h>                  // Include ddk function header file

#include "Vm.h"                     // Include root header file

#include "Scanner.h"                // Include scanner header file

#include "DisassemblerInterface.h"

/******************************************************************************
*                                                                             *
*   Global Variables                                                          *
*                                                                             *
******************************************************************************/
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

extern DWORD CurrentInstrLen;

#define RETURN_ADVANCE_EIP              0
#define RETURN_KEEP_EIP                 1

typedef DWORD (*TFaulting)(DWORD flags, BYTE bModrm, DWORD address, TMEM *pMem);

//                            0   -1-    -2-   3   -4-
static DWORD MaskTable[5] = { 0, 0xFF, 0xFFFF, 0, 0xFFFFFFFF };

/******************************************************************************
*                                                                             *
*   Functions                                                                 *
*                                                                             *
******************************************************************************/


DWORD MemoryRead(DWORD flags, DWORD offset, DWORD data_len)
{
    TMEM *pMem;
    WORD wSel;
    DWORD i, *pValue;

    // Get the effective address from the selector and offset

    wSel = LOWORD( *(((DWORD *)&pMonitor->guestTSS.es) + (flags >> 16)) );

    // wSel either represent the segment or selector, depending on the execution mode

    if( pMonitor->guestTSS.eflags & VM_MASK )
    {
        // wSel contains segment value: running either `real` mode or v86 mode

        offset += wSel << 4;
    }
    else
    {
        ASSERT(0);      // Protected mode
    }

    // The source memory address may be physical memory or faulting region

    pMem = &Device.MEM[0];

    for( i=0; i<Device.MemTop; i++, pMem++ )
    {
        if( (offset >= pMem->BaseAddress) &&
            (offset <= pMem->LastAddress) )
        {
            return( (pMem->pReadCallback)( offset, data_len ) );
        }
    }

    // The source memory was a regular linear address

    if((pValue = (DWORD *)VmLinearToDriverLinear(offset))==NULL )
        ASSERT(0);

    return( *pValue & MaskTable[data_len] );
}


void MemoryWrite(DWORD flags, DWORD offset, DWORD value, DWORD data_len)
{
    TMEM *pMem;
    WORD wSel;
    DWORD i, *pValue;

    // Get the effective address from the selector and offset

    wSel = LOWORD( *(((DWORD *)&pMonitor->guestTSS.es) + (flags >> 16)) );

    // wSel either represent the segment or selector, depending on the execution mode

    if( pMonitor->guestTSS.eflags & VM_MASK )
    {
        // wSel contains segment value: running either `real` mode or v86 mode

        offset += wSel << 4;
    }
    else
    {
        ASSERT(0);      // Protected mode
    }

    // The target memory address may be physical memory or faulting region

    pMem = &Device.MEM[0];

    for( i=0; i<Device.MemTop; i++, pMem++ )
    {
        if( (offset >= pMem->BaseAddress) &&
            (offset <= pMem->LastAddress) )
        {
            (pMem->pWriteCallback)( offset, value, data_len );
            return;
        }
    }

    // The target memory was a regular linear address

    if((pValue = (DWORD *)VmLinearToDriverLinear(offset))==0 )
        ASSERT(0);

    switch( data_len )
    {
        case 1:     *(BYTE *)pValue = (BYTE) value;   break;
        case 2:     *(WORD *)pValue = (WORD) value;   break;
        case 4:     *pValue = value;   break;
        default:    ASSERT(0);
    }
}


//*****************************************************************************
//*                                                                           *
//*                     STOSB / STOSW / STOSD                                 *
//*                                                                           *
//*****************************************************************************

static DWORD common_fn_stos_a32( DWORD flags, DWORD value, DWORD data_len, int delta )
{
    if( flags & DIS_REP )
loopback:
        if( pMonitor->guestTSS.ecx==0 )
            return( 0 );

    MemoryWrite(flags, pMonitor->guestTSS.edi, value, data_len);

    pMonitor->guestTSS.edi += delta;

    if( flags & DIS_REP )
    {
        pMonitor->guestTSS.ecx -= 1;
        goto loopback;
    }

    return( RETURN_ADVANCE_EIP );
}

static DWORD common_fn_stos_a16( DWORD flags, DWORD value, DWORD data_len, int delta )
{
    if( flags & DIS_REP )
loopback:
        if( (*(WORD *)&pMonitor->guestTSS.ecx)==0 )
            return( 0 );

    MemoryWrite(flags, *(WORD *)&pMonitor->guestTSS.edi, value, data_len);

    *(WORD *)&pMonitor->guestTSS.edi += (short) delta;

    if( flags & DIS_REP )
    {
        (*(WORD *)&pMonitor->guestTSS.ecx)--;
        goto loopback;
    }

    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_stosb(DWORD flags, BYTE bModrm, DWORD address, TMEM *pMem)
{
    int delta;
    ASSERT(!(flags & DIS_SEGOVERRIDE));     // Segment override not possible
    flags &= 0x0000FFFF;                    // .. defaults to ES
    ASSERT(!(flags & DIS_REPNE));           // This form of REP prefix not allowed

    if( pMonitor->guestTSS.eflags & DF_MASK )
        delta = -1;
    else
        delta = 1;
                                                            // STOSB always
    if( flags & DIS_ADDRESS32 )
        return( common_fn_stos_a32(flags, pMonitor->guestTSS.eax & 0xFF, 1, delta ));
    else
        return( common_fn_stos_a16(flags, pMonitor->guestTSS.eax & 0xFF, 1, delta ));
}

//=============================================================================
static DWORD fn_stosv(DWORD flags, BYTE bModrm, DWORD address, TMEM *pMem)
{
    DWORD value, size;
    int delta;
    ASSERT(!(flags & DIS_SEGOVERRIDE));     // Segment override not possible
    flags &= 0x0000FFFF;                    // .. defaults to ES
    ASSERT(!(flags & DIS_REPNE));           // This form of REP prefix not allowed

    if( flags & DIS_DATA32 )
        value = pMonitor->guestTSS.eax, size = 4;           // STOSD
    else
        value = pMonitor->guestTSS.eax & 0xFFFF, size = 2;  // STOSW

    if( pMonitor->guestTSS.eflags & DF_MASK )
        delta = -(int)size;
    else
        delta = size;

    if( flags & DIS_ADDRESS32 )
        return( common_fn_stos_a32(flags, value, size, delta ));
    else
        return( common_fn_stos_a16(flags, value, size, delta ));
}


//*****************************************************************************
//*                                                                           *
//*                     MOVSB / MOVSW / MOVSD                                 *
//*                                                                           *
//*****************************************************************************

static DWORD common_fn_movs_a32( DWORD flags, DWORD data_len, int delta )
{
    if( flags & DIS_REP )
loopback:
        if( pMonitor->guestTSS.ecx==0 )
            return( 0 );

    MemoryWrite(flags & 0x0000FFFF,         // Default ES for destination
                pMonitor->guestTSS.edi,
                MemoryRead(flags, pMonitor->guestTSS.esi, data_len),
                data_len);

    pMonitor->guestTSS.edi += delta;
    pMonitor->guestTSS.esi += delta;

    if( flags & DIS_REP )
    {
        pMonitor->guestTSS.ecx -= 1;
        goto loopback;
    }

    return( RETURN_ADVANCE_EIP );
}

static DWORD common_fn_movs_a16( DWORD flags, DWORD data_len, int delta )
{
    if( flags & DIS_REP )
loopback:
        if( (*(WORD *)&pMonitor->guestTSS.ecx)==0 )
            return( 0 );

    MemoryWrite(flags & 0x0000FFFF,         // Default ES for destination
                *(WORD *)&pMonitor->guestTSS.edi,
                MemoryRead(flags, *(WORD *)&pMonitor->guestTSS.esi, data_len),
                data_len);

    *(WORD *)&pMonitor->guestTSS.edi += (short) delta;
    *(WORD *)&pMonitor->guestTSS.esi += (short) delta;

    if( flags & DIS_REP )
    {
        (*(WORD *)&pMonitor->guestTSS.ecx)--;
        goto loopback;
    }

    return( RETURN_ADVANCE_EIP );
}

//=============================================================================
static DWORD fn_movsb(DWORD flags, BYTE bModrm, DWORD address, TMEM *pMem)
{
    int delta;
    ASSERT(!(flags & DIS_SEGOVERRIDE));     // Segment override not possible
    ASSERT(!(flags & DIS_REPNE));           // This form of REP prefix not allowed

    if( pMonitor->guestTSS.eflags & DF_MASK )
        delta = -1;
    else
        delta = 1;
                                                    // MOVSB always
    if( flags & DIS_ADDRESS32 )
        return( common_fn_movs_a32(flags, 1, delta ));
    else
        return( common_fn_movs_a16(flags, 1, delta ));
}

//=============================================================================
static DWORD fn_movsv(DWORD flags, BYTE bModrm, DWORD address, TMEM *pMem)
{
    DWORD size;
    int delta;
    ASSERT(!(flags & DIS_SEGOVERRIDE));     // Segment override not possible
    ASSERT(!(flags & DIS_REPNE));           // This form of REP prefix not allowed

    if( flags & DIS_DATA32 )
        size = 4;                                   // MOVSD
    else
        size = 2;                                   // MOVSW

    if( pMonitor->guestTSS.eflags & DF_MASK )
        delta = -(int)size;
    else
        delta = size;

    if( flags & DIS_ADDRESS32 )
        return( common_fn_movs_a32(flags, size, delta ));
    else
        return( common_fn_movs_a16(flags, size, delta ));
}


/******************************************************************************
*                                                                             *
*   Array of pointers to functions above                                      *
*                                                                             *
******************************************************************************/

TFaulting Faulting[] = {
    fn_stosb,             // 0x80     AA
    fn_stosv,             // 0x81     AB
    fn_movsb,             // 0x82     A4
    fn_movsv,             // 0x83     A5
};
