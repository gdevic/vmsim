/******************************************************************************
*                                                                             *
*   Module:     Memory.cpp                                                    *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       3/16/2000                                                     *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the application code for initialization
        and shut down of a VM.

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 3/16/2000  Original                                             Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Include Files                                                             *
******************************************************************************/
#include "stdafx.h"                     // Include root MFC header file

#include <assert.h>                     // Include assert support

#include "vmGUI.h"                      // ?

#include <winioctl.h>                   // Include Windows IO controls

#include "Vm.h"                         // Include root header file

#include "IoCtl.h"                      // Include IO control defines

#include "Simulator.h"                  // Include Simulator class header file

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

/******************************************************************************
*                                                                             *
*   TVM *DriverAllocTVM()                                                     *
*                                                                             *
*******************************************************************************
*
*   This function calls the driver to allocate the main TVM structure.
*
*   Returns:
*       pointer to TVM structure as mapped in the application address space
*       NULL if failed
*
******************************************************************************/
static TVM *DriverAllocTVM()
{
    OVERLAPPED overlap = { 0 };
    DWORD status, nb;
    TVM *pVM;

    DeviceIoControl( pTheApp->m_pSimulator->sys_handle, VMSIM_AllocTVM,
        NULL, 0,
        (LPVOID) &pVM, 4,   // address is in pVM
        &nb, &overlap );    // nb ignored
    status = GetOverlappedResult(pTheApp->m_pSimulator->sys_handle, &overlap, &nb, TRUE);
    if(!status)
    {
        status = GetLastError();
        ASSERT(0);
    }

    return( pVM );
}


/******************************************************************************
*                                                                             *
*   int DriverFreeTVM(TVM *pVM)                                               *
*                                                                             *
*******************************************************************************
*
*   This function frees the VM memory allocated by DriverAllocTVM() call.
*
*   Where:
*       memory address returned by DriverAllocTVM() call.
*
*   Returns:
*       0 on success
*       1 on failure
*
******************************************************************************/
int DriverFreeTVM(TVM *pVM)
{
    OVERLAPPED overlap = { 0 };
    DWORD status, nb;

    DeviceIoControl( pTheApp->m_pSimulator->sys_handle, VMSIM_FreeTVM,
        (LPVOID) &pVM, sizeof(TVM),
        NULL, 0,
        &nb, &overlap );    // nb ignored
    status = GetOverlappedResult(pTheApp->m_pSimulator->sys_handle, &overlap, &nb, TRUE);
    if(!status)
    {
        status = GetLastError();
        ASSERT(0);
    }

    return( 0 );
}


/******************************************************************************
*                                                                             *
*   int DriverMapROM()                                                        *
*                                                                             *
*******************************************************************************
*
*   This function maps ROM image into the virtual machine address space.
*   Usually, there are 2 images: system BIOS and VGA BIOS.
*
*   Where:
*       pROM is the application address of a ROM image to map
*       dwSize is the size in bytes of that image
*       dwMapTo is the physical address in the virtual machine where to map it
*
*   Returns:
*       0 on success
*       1 on failure
*
******************************************************************************/
static int DriverMapROM(PVOID pROM, DWORD dwSize, DWORD dwMapTo)
{
    OVERLAPPED overlap = { 0 };
    DWORD status, nb;
    DWORD Info[2];

    Info[0] = dwSize;
    Info[1] = dwMapTo;

    DeviceIoControl( pTheApp->m_pSimulator->sys_handle, VMSIM_MapROM,
        (LPVOID) &Info[0], sizeof(Info),
        pROM, dwSize,
        &nb, &overlap );    // nb ignored
    status = GetOverlappedResult(pTheApp->m_pSimulator->sys_handle, &overlap, &nb, TRUE);
    if(!status)
    {
        status = GetLastError();
        ASSERT(0);
    }

    return( 0 );
}


/******************************************************************************
*                                                                             *
*   TVM * Init_VM()                                                           *
*                                                                             *
*******************************************************************************
*
*   This function sets, allocates and initializes all the data structures
*   needed to run a virtual machine.
*
*   Where:
*
*   Returns:
*       Pointer to a TVM structure
*       NULL if failed
*
******************************************************************************/
TVM * Init_VM()
{
    OVERLAPPED overlap = { 0 };
    DWORD status, nb;
    HMODULE hInstance;
    HRSRC hFound;
    HGLOBAL hRes;
    LPVOID lpBuff;
    DWORD dwResourceSize, InitOK;
    TVM *pVM;


    // Allocate memory for the root VM structure from the device driver

    pVM = DriverAllocTVM();

    if( pVM != NULL )
    {
        //=====================================================================
        // Initialize VM
        //=====================================================================

        // Find the initial host memory hole for cross-task monitor pages

        DeviceIoControl( pTheApp->m_pSimulator->sys_handle, VMSIM_FindNTHole,
            NULL, 0,
            (LPVOID) &pVM->dwVmMonitorLinear, 4,
            &nb, &overlap );    // nb ignored
        status = GetOverlappedResult(pTheApp->m_pSimulator->sys_handle, &overlap, &nb, TRUE);
        if(!status)
        {
            status = GetLastError();
            ASSERT(0);
        }

        ASSERT(pVM->dwVmMonitorLinear);

        //---------------------------------------------------------------------
        // Read the amount of VM physical memory from the config data

        pVM->dwVmMemorySize   = pTheApp->m_VMConfig.PhysMem << 20;

        // Find out the amount of RAM on a host machine and adjust the VM
        // accordingly

        pVM->dwHostMemorySize = pTheApp->m_HostPhysMem << 20;

        // Enable warnings and trace outputs into a debugger window

#ifdef _DEBUG
        pVM->fEnableWarnings = TRUE;
        pVM->fEnableTrace    = TRUE;
#else
        pVM->fEnableWarnings = FALSE;
        pVM->fEnableTrace    = FALSE;
#endif
        // Initialize CMOS data from a VM configuration file

        ASSERT(sizeof(pVM->CMOS)==sizeof(pTheApp->m_VMConfig.CMOS));

        memcpy(pVM->CMOS, pTheApp->m_VMConfig.CMOS, sizeof(pTheApp->m_VMConfig.CMOS));
#if 0
            // Copy (optional) hard disk parameters

            for( int i=0; i<4; i++)
        {
            pVM->Ide[i].heads = pTheApp->m_VMConfiguration.Ide[i].heads;
            pVM->Ide[i].cylinders = pTheApp->m_VMConfiguration.Ide[i].cylinders;
            pVM->Ide[i].spt = pTheApp->m_VMConfiguration.Ide[i].spt;
        }
#endif
        // Clean statistics counters
#if INCLUDE_STATS
        memset(&pVM->Stats, 0, sizeof(TStats));
#endif

        // Clean up monochrome and vga text display data arrays

        memset(pVM->MdaTextBuffer, 0x20, sizeof(pVM->MdaTextBuffer));
        memset(pVM->VgaTextBuffer, 0x20, sizeof(pVM->VgaTextBuffer));

        //---------------------------------------------------------------------
        // Call device driver to allocate and initialize its memory and working
        // environment for the current virtual machine.
        //
        // Some of it will be based on the parameters set above.

        InitOK = 1;
        DeviceIoControl( pTheApp->m_pSimulator->sys_handle, VMSIM_InitVM,
            NULL, 0,
            (LPVOID) &InitOK, 4,
            &nb, &overlap );
        status = GetOverlappedResult(pTheApp->m_pSimulator->sys_handle, &overlap, &nb, TRUE);
        if(!status)
        {
            status = GetLastError();
            ASSERT(0);
        }

        if( InitOK != 1 )
        {
            //------------------------------------------------------------------------
            // Load resource and map ROM images: There are 2 images to map:
            //  IDR_VGAROM  -> C000:0000 (32K)
            //  IDR_SYSBIOS -> F000:0000 (64K)
            //------------------------------------------------------------------------

            hInstance = AfxGetResourceHandle();
            hFound = FindResource(hInstance, MAKEINTRESOURCE(IDR_VGAROM), "BINARY");
            ASSERT(hFound);
            hRes = LoadResource(hInstance, hFound);
            ASSERT(hRes);
            dwResourceSize = SizeofResource(hInstance,hFound);
            ASSERT(dwResourceSize==32*1024);
            lpBuff = LockResource(hRes);

            DriverMapROM(lpBuff, dwResourceSize, 0xC0000);

            // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

            hFound = FindResource(hInstance, MAKEINTRESOURCE(IDR_SYSBIOS), "BINARY");
            ASSERT(hFound);
            hRes = LoadResource(hInstance, hFound);
            ASSERT(hRes);
            dwResourceSize = SizeofResource(hInstance,hFound);
            ASSERT(dwResourceSize==64*1024);
            lpBuff = LockResource(hRes);

            DriverMapROM(lpBuff, dwResourceSize, 0xF0000);

            //------------------------------------------------------------------------
            // Load test stub resource and map it to address 0. The driver knows
            // it is a test code, so it will set virtual CS:EIP to execute it.
            //  IDR_TESTSTUB  -> 0000:0000
            //------------------------------------------------------------------------
#if 1
            hFound = FindResource(hInstance, MAKEINTRESOURCE(IDR_TESTSTUB), "BINARY");
            ASSERT(hFound);
            hRes = LoadResource(hInstance, hFound);
            ASSERT(hRes);
            dwResourceSize = SizeofResource(hInstance,hFound);
            lpBuff = LockResource(hRes);

            DriverMapROM(lpBuff, dwResourceSize, 0x0);
#endif
            // Return with the application address of the VM structure

            return( pVM );
        }
        // Failed somewhere initializing VM... release what we've got.

        DriverFreeTVM(pVM);
    }

    return( NULL );
}


/******************************************************************************
*                                                                             *
*   void Free_VM( TVM* pVM )                                                  *
*                                                                             *
*******************************************************************************
*
*   This function frees and deallocates a VM.  It is used when a current VM
*   is shut down and closed.
*
*   Where:
*       pVM is the address of the VM structure (can be NULL)
*
******************************************************************************/
void Free_VM( TVM* pVM )
{
    if( pVM )
    {
        // Free VM structure.  Since this is a basic data structure, the
        // driver will try to free all other memory blocks allocated and
        // completely clean up.  This call is an effective 'Shut Down.'

        DriverFreeTVM(pVM);
    }
}

