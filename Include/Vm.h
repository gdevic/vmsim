/******************************************************************************
*                                                                             *
*   Module:     Vm.h                                                          *
*                                                                             *
*   Revision:   2.00                                                          *
*                                                                             *
*   Date:       3/10/2000                                                     *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This file is a root header file for inclusion of major VM defines
        and structures.

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 3/10/2000  Original                                             Goran Devic *
* 5/22/2000  New header tree with this file as the root           Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Important Defines                                                         *
******************************************************************************/
#ifndef _VM_H_
#define _VM_H_

/******************************************************************************
*                                                                             *
*   Global Defines, Variables and Macros                                      *
*                                                                             *
*   These are defined first so they can be used from any leaf header file     *
*                                                                             *
******************************************************************************/

// Linux needs 1036 entries in the GDT table (1024 for processes and 12 kernel)
#define GDT_MAX             (1036 + 10) // Number of GDT entries (VM + Monitor)
#define BRIDGE_CODE_SIZE    4096        // Maximum size of the monitor code
#define BRIDGE_STACK_SIZE   256         // Size of the shared ring-0 stack
#define MAX_IOPERM          65536       // Guest TSS' IO permission bitmap size
#define MAX_META_DATA       32          // Number of meta-structures
#define MAX_VDD             32          // Number of virtual devices
#define MAX_IO_HND          32          // Number of IO port handlers
#define MAX_PCI_DEVICES     32          // Maximum PCI devices (const, spec.)
#define MAX_MEM_HND         16          // Maximum memory arena handlers
#define CMOS_SIZE           128         // RTC CMOS size in bytes
#define INCLUDE_STATS       1           // Enable (1) or disable various counters
#define MAX_FLOPPY_TRACKS   80          // Floppy: Number of tracks (cylinders)
#define MAX_FLOPPY_HEADS    2           // Floppy: Number of heads
#define MAX_FLOPPY_SECTORS  18          // Floppy: Number of sectors per track

/******************************************************************************
*                                                                             *
*   Include Files                                                             *
*                                                                             *
******************************************************************************/
// Make sure we have consistent structure packing (dword aligned)
#pragma pack( push, enter_vm )
#pragma pack( 4 )
//-----------------------------------------------------------------------------

// Include following header files for both device driver and the application

#include "Types.h"                      // Include basic data types
#include "Statistics.h"                 // Include statistic and couters
#include "Intel.h"                      // Include Intel specific defines
#include "Interface.h"                  // Include virtual device interfaces

typedef struct
{
    int     hHandle;                    // Windows file handle, >0 for enabled disk
    DWORD   heads;                      // Heads
    DWORD   cylinders;                  // Cylinders
    DWORD   tracks;                     // Tracks on a cylinder
    DWORD   spt;                        // Sectors per track

} THdd;

//-----------------------------------------------------------------------------
// A global structure defining the VM.
//-----------------------------------------------------------------------------
typedef struct
{
    //-------------------------------------------------------------------------
    // Initialization values that need to be set before running a VM
    //-------------------------------------------------------------------------
    DWORD dwVmMemorySize;               // Guest VM physical memory size in bytes
    DWORD dwHostMemorySize;             // Host memory size in bytes
    DWORD dwVmMonitorLinear;            // Where to map monitor inside VM space

    //-------------------------------------------------------------------------
    // Application-driver communication data area:
    //-------------------------------------------------------------------------
    // Debug level for debug prints and trace facility
    // ------------------------------------------------------------------------
    DWORD fEnableWarnings;
    DWORD fEnableTrace;

    // ------------------------------------------------------------------------
    // CMOS data bytes
    // ------------------------------------------------------------------------
    BYTE CMOS[ CMOS_SIZE ];

    // ------------------------------------------------------------------------
    // Monochrome display adapter data: 32 K text buffer
    // ------------------------------------------------------------------------
    BYTE MdaTextBuffer[ 0xB7FFF - 0xB0000 + 1];

    // ------------------------------------------------------------------------
    // VGA display adapter data: 32 K text buffer
    // ------------------------------------------------------------------------
    BYTE VgaTextBuffer[ 0xBFFFF - 0xB8000 + 1];

    // ------------------------------------------------------------------------
    // Optional statistics counters
    // ------------------------------------------------------------------------
#if INCLUDE_STATS
    TStats Stats;
#endif

} TVM;


///////////////////////////////////////////////////////////////////////////////
// Include following headers only if we are compiling a device driver
///////////////////////////////////////////////////////////////////////////////

#ifndef GUI
#include "Debug.h"                      // Include debug tools and messaging
#include "Memory.h"                     // Include memory management header
#include "Monitor.h"                    // Include monitor header file
#include "CPU.h"                        // Include virtual CPU header file
#include "Device.h"                     // Include header for virtual devices
#include "Globals.h"                    // Include global functions and data
#endif


//-----------------------------------------------------------------------------
// Restore structure packing value
#pragma pack( pop, enter_vm )


#endif //  _VM_H_
