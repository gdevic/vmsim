/******************************************************************************
*                                                                             *
*   Module:     Memory.c                                                      *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       4/19/2000                                                     *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the code for virtualizing memory access.

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 4/19/2000  Original                                             Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Include Files                                                             *
******************************************************************************/
#include <strmini.h>                // Include driver level C functions

#include <ntddk.h>                  // Include ddk function header file

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

/******************************************************************************
*                                                                             *
*   Functions                                                                 *
*                                                                             *
******************************************************************************/

static int MemoryInitialize(void);

static DWORD MemDefaultREAD(DWORD address, DWORD data_len);
static void MemDefaultWRITE(DWORD address, DWORD data, DWORD data_len);


/******************************************************************************
*                                                                             *
*   void MemoryRegister(TVDD *pVdd)                                           *
*                                                                             *
*******************************************************************************
*
*   Registration function for the memory trapping subsystem.
*
*   Where:
*       pVdd is the address of the virtual device desctiptor to fill in
*
******************************************************************************/
void MemoryRegister(TVDD *pVdd)
{
    pVdd->Initialize = MemoryInitialize;
    pVdd->Name = "Memory subsystem";
}


/******************************************************************************
*                                                                             *
*   int MemoryInitialize()                                                    *
*                                                                             *
*******************************************************************************
*
*   Initialization function for the memory trapping subsystem.
*
*   Returns:
*       0 on success
*       nonzero on fail
*
******************************************************************************/
static int MemoryInitialize()
{
    TRACE(("MemoryInitialize()"));

    return( 0 );
}


/******************************************************************************
*                                                                             *
*   int RegisterMemoryCallback( DWORD physical_address, DWORD last_address,   *
*           TMEM_READ_Callback fnREAD, TMEM_WRITE_Callback fnWRITE )          *
*                                                                             *
*******************************************************************************
*
*   This function registers memory trap regions.
*
*   Where:
*       physical_address is the starting address to hook
*       last_address is the last physical address defining a memory arena
*       fnREAD is a callback function when reading from a memory location
*       fnWRITE is a callback function when writing to a memory location
*
*   Returns:
*       0 if the registration succeeds
*       non-zero if the memory arena can not be registered
*           -1 : specified memory range is already registered
*           -2 : bad parameter to a call
*           -3 : internal (out of MEM structures)
*
******************************************************************************/
int RegisterMemoryCallback( DWORD physical_address, DWORD last_address,
        TMEM_READ_Callback fnREAD, TMEM_WRITE_Callback fnWRITE )
{
    DWORD i;
    TMEM *pMem;
    TPage *pPage;

    ASSERT(physical_address<=last_address);

    if( physical_address<=last_address )
    {
        // Make sure that requested memory range is not already registered

        for( i=0; i<Device.MemTop; i++ )
        {
            if( (physical_address >= Device.MEM[i].BaseAddress) &&
                (physical_address <= Device.MEM[i].LastAddress) )
                {
                    WARNING(("Memory region already registered"));
                    return( -1 );
                }

            if( (last_address >= Device.MEM[i].BaseAddress) &&
                (last_address <= Device.MEM[i].LastAddress) )
                {
                    WARNING(("Memory region already registered"));
                    return( -1 );
                }
        }

        // Register memory range within the MEM array

        ASSERT(Device.MemTop<MAX_MEM_HND);

        if( Device.MemTop<MAX_MEM_HND )
        {
            INFO(("Registering memory access handler: %08X - %08X", physical_address, last_address ));

            pMem = &Device.MEM[ Device.MemTop ];

            pMem->BaseAddress = physical_address;
            pMem->LastAddress = last_address;

            if( fnREAD != NULL )
                pMem->pReadCallback = fnREAD;
            else
                pMem->pReadCallback = MemDefaultREAD;

            if( fnWRITE != NULL )
                pMem->pWriteCallback = fnWRITE;
            else
                pMem->pWriteCallback = MemDefaultWRITE;

            Device.MemTop++;

            // Finally, set up the pages that were requested to be
            // not-present within our VM paging system

            pPage = &Mem.pPagePTE[ physical_address >> 12 ];
            i = (last_address - physical_address) >> 12;

            // TODO: Speed optimization: Index field of a device memory PTE could
            //       index the Device[] memory since those pages are marked not
            //       present, so we can keep additional info in the PTE record
            do
            {
                pPage->fPresent = FALSE;
                pPage++;
            }
            while( i-- );

            return( 0 );
        }

        WARNING(("Memory array too small"));
        return( -3 );
    }

    WARNING(("Bad parameter to a RegisterMemoryCallback()"));
    return( -2 );
}


//*****************************************************************************
//
//      Default function callbacks that are used instead of read or write
//      function if NULL is passed as function argument to
//      RegisterMemoryCallback().
//
//*****************************************************************************

static DWORD MemDefaultREAD(DWORD address, DWORD data_len)
{
    TRACE(("MemDefaultREAD(address=%08X, data_len=%d)", address, data_len));

    ASSERT(0);

    return( 0xFFFFFFFF );
}

static void MemDefaultWRITE(DWORD address, DWORD data, DWORD data_len)
{
    TRACE(("MemDefaultWRITE(address=%08X, data=%08X data_len=%d)", address, data, data_len));

    ASSERT(0);
}

