/******************************************************************************
*                                                                             *
*   Module:     MemoryAccess.c												  *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       7/21/2000													  *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the code for the (legacy) handling of the
        memory access that occurred on a region that was marked as trapping.

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 7/21/2000	 Original                                             Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Include Files                                                             *
******************************************************************************/
#include <strmini.h>                // Include driver level C functions

#include <ntddk.h>                  // Include ddk function header file

#include "Vm.h"                     // Include root header file

#include "DisassemblerDefines.h"    // Include disassembler tools

/******************************************************************************
*                                                                             *
*   Global Variables                                                          *
*                                                                             *
******************************************************************************/

typedef struct
{
    TPage SparePage[2];                 // Substitute spare pages
    TPage *pPage;                       // Base address of the page in VM PTE
    DWORD fault_address;                // Current access: address
    DWORD data_len;                     // Current access: data width
    TMEM *pMem;                         // Current access: memory handler

} TAccess;


TAccess pf;         // Current state of the memory access procedure

/******************************************************************************
*                                                                             *
*   Local Defines, Variables and Macros                                       *
*                                                                             *
******************************************************************************/


/******************************************************************************
*                                                                             *
*   Functions                                                                 *
*                                                                             *
******************************************************************************/

static void PostAccessRead(void);
static void PostAccessWrite(void);

//*****************************************************************************
//*                                                                           *
//*                     LEGACY EXECUTION                                      *
//*                                                                           *
//*****************************************************************************
//
// pMonitor->guestCR2 contains a linear address of the target memory access
// CPU state is ready to repeat the instruction

/******************************************************************************
*                                                                             *
*   void InitMemoryAccess()                                                   *
*                                                                             *
*******************************************************************************
*
*   Initializes memory access subsystem
*
******************************************************************************/
void InitMemoryAccess()
{
    memset( &pf, 0, sizeof(TAccess) );

    // Spare pages are already allocated and we need to set up their PTE
    // within our TAccess structure

    pf.SparePage[0].fPresent  = 1;
    pf.SparePage[0].fUser     = 1;
    pf.SparePage[0].fWriteThr = 1;
    pf.SparePage[0].fNoCache  = 1;
    pf.SparePage[0].Flags     = 0;
    pf.SparePage[0].Index     = Mem.MemAccess.physical[0];

    pf.SparePage[1].fPresent  = 1;
    pf.SparePage[1].fUser     = 1;
    pf.SparePage[1].fWriteThr = 1;
    pf.SparePage[1].fNoCache  = 1;
    pf.SparePage[1].Flags     = 0;
    pf.SparePage[1].Index     = Mem.MemAccess.physical[1];
}



/******************************************************************************
*                                                                             *
*   TPostState                                                                *
*   MemoryAccess( BYTE access, BYTE flags, DWORD dwLinVM, TMEM *pMem)         *
*                                                                             *
*******************************************************************************
*
*   Executes memory access
*
*   Where:
*       access are access flags from the instruction disassembly table
*              containing the R/W bits in the upper nibble and data_len code
*              in the lower nibble
*
*       data_len is the effective data width
*
*       pMem is the pointer to the registered memory arena
*
*   Implicit:
*
*       pMonitor->guestCR2 contains the linear address in the VM space of
*               the access (fault)
*
*   Returns:
*       Pointer to a post-access function
*
******************************************************************************/
TPostState MemoryAccess( BYTE access, BYTE data_len, TMEM *pMem)
{
    DWORD value;

    pf.fault_address = pMonitor->guestCR2;

    // Get the access data width of the current instruction

    pf.data_len = data_len;

    // Get the original PTE base address so we can swap PTE entries

    pf.pPage = &Mem.pPagePTE[ pMonitor->guestCR2 >> 12 ];

    // Store the pMem pointer

    pf.pMem = pMem;

    if( access & INSTR_READ )           // Instruction reads memory
    {
        pf.SparePage[0].fWrite = 0;     // Disallow writing
        pf.SparePage[1].fWrite = 0;

        // TODO: what if the first page is on the last address, so the
        // inserting second would go over the PTE limit?

        // TODO: Optimization: insert only one page unless we have a boundary
        // access condition?

        *pf.pPage = pf.SparePage[0];    // Set our pages into the region
        *(pf.pPage + 1) = pf.SparePage[1];

        // Call the virtual device read function to get the value

        value = (pMem->pReadCallback)( pf.fault_address, pf.data_len );

        // Store the value into our spare pages

        *(DWORD *) (Mem.MemAccess.linear + (pf.fault_address & 0xFFF)) = value;

        // After a single step, return to finalize the instruction

        return( &PostAccessRead );
    }
    else
    if( access & INSTR_WRITE )          // Instruction writes to memory
    {
        pf.SparePage[0].fWrite = 1;     // Allow writing
        pf.SparePage[1].fWrite = 1;

        // TODO: what if the first page is on the last address, so the
        // inserting second would go over the PTE limit?

        // TODO: Optimization: insert only one page unless we have a boundary
        // access condition?

        *pf.pPage = pf.SparePage[0];    // Set our pages into the region
        *(pf.pPage + 1) = pf.SparePage[1];

        // After a single step, return to finalize the instruction

        return( &PostAccessWrite );
    }
    else                                // Instruction is read-modify-write
    {
        pf.SparePage[0].fWrite = 1;     // Allow writing
        pf.SparePage[1].fWrite = 1;

        // TODO: what if the first page is on the last address, so the
        // inserting second would go over the PTE limit?

        // TODO: Optimization: insert only one page unless we have a boundary
        // access condition?

        *pf.pPage = pf.SparePage[0];    // Set our pages into the region
        *(pf.pPage + 1) = pf.SparePage[1];

        // Call the virtual device read function to get the value

        value = (pMem->pReadCallback)( pf.fault_address, pf.data_len );

        // Store the value into our spare pages

        *(DWORD *) (Mem.MemAccess.linear + (pf.fault_address & 0xFFF)) = value;

        // After a single step, return to finalize the instruction

        return( &PostAccessWrite );
    }
}


// Coming back from the instruction that read a value from a memory region.  We
// are here after a single step instruction.

static void PostAccessRead(void)
{
    // Set the page not present in the original PTE

    // TODO: what if the first page is on the last address, so the
    // inserting second would go over the PTE limit?

    pf.pPage->fPresent = FALSE;
    (pf.pPage+1)->fPresent = FALSE;
}


// Coming back from the instruction that wrote a value to a memory region.  We
// are here after a single step instruction.

static void PostAccessWrite(void)
{
    DWORD value;

    // Set the page not present in the original PTE

    // TODO: what if the first page is on the last address, so the
    // inserting second would go over the PTE limit?

    pf.pPage->fPresent = FALSE;
    (pf.pPage+1)->fPresent = FALSE;

    // Fetch the value that was written by the write instruction

    switch( pf.data_len )
    {
        case 1:                 // Byte access
            value = *(DWORD *) (Mem.MemAccess.linear + (pf.fault_address & 0xFFF));
            break;

        case 2:                 // Word access
            value = *(WORD *) (Mem.MemAccess.linear + (pf.fault_address & 0xFFF));
            break;

        case 4:                 // Dword access
            value = *(BYTE *) (Mem.MemAccess.linear + (pf.fault_address & 0xFFF));
            break;

        default:                // Huh?
            ASSERT(0);
    }

    // Call the virtual device write function

    (pf.pMem->pWriteCallback)( pf.fault_address, value, pf.data_len );
}

