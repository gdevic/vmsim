/******************************************************************************
*                                                                             *
*   Module:     Memory.h                                                      *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       5/23/2000                                                     *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This is a header file containing driver memory management structures

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 5/23/2000  Original                                             Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Important Defines                                                         *
******************************************************************************/
#ifndef _MEMORY_H_
#define _MEMORY_H_

/******************************************************************************
*                                                                             *
*   Global Defines, Variables and Macros                                      *
*                                                                             *
******************************************************************************/

//-----------------------------------------------------------------------------
// Define memory descriptor structure entry (Monitor, VM, PTE)
//-----------------------------------------------------------------------------
typedef struct
{
    DWORD pages;                    // Number of elements in the array
    DWORD linear;                   // Linear ddress of the memory block
    DWORD *physical;                // Array of physical page numbers

} TMemTrack;


//-----------------------------------------------------------------------------
// Define Meta-Data control structure: this structure had pointers to a page
// that contains modified code page, another that contains meta data for that
// code page and the VM index of that code page
//-----------------------------------------------------------------------------
typedef struct TagMeta
{
    struct TagMeta *next;           // next TMeta structure to make...
    struct TagMeta *prev;           // previous for doubly-linked list

    TMemTrack MemPage;              // 2 pages: modified code & meta page
    TPage VmCodePage;               // Vm (pseudo) physical code page that this shadows
                                    // fPresent is 1 for valid mapping, 0 for not used
    TPage *pPTE;                    // Pointer to a monitor PTE describing the VM frame
    // ASSUMPTION: A meta page (code, ring0) will never be mapped to more than
    //             one linear address inside the guest VM
} TMeta;


typedef struct
{
    TMemTrack MemVM;                // Memory descriptor for VM pseudo-physical
    TMemTrack MemPTE;               // Memory descriptor for VM page table
    TMemTrack MemMonitor;           // Memory descriptor for monitor pages
    TMemTrack MemAccess;            // Memory descriptor for space access pages

    DWORD    *physLookup;           // Physical page lookup array
    BOOL      fA20;                 // A20 line is active (1) or disabled (0)
    TPage    *pPagePTE;             // Points to MemPTE->linear for simpler access

    TMeta Meta[ MAX_META_DATA ];    // Meta data control structures
    TMeta *pMetaHead;               // Logically first meta structure
    TMeta *pMetaTail;               // Logically last meta structure
    TMeta **metaLookup;             // Maps VM physical into meta-structures
    BYTE  *pPageStamp;              // Array of page stamps
//#define PAGESTAMP_TSS_MASK      0x01    // Page contains VM TSS table
//#define PAGESTAMP_GDT_MASK      0x02    // Page contains VM GDT table
//#define PAGESTAMP_LDT_MASK      0x04    // Page contains VM LDT table
//#define PAGESTAMP_IDT_MASK      0x08    // Page contains VM IDT table
//#define PAGESTAMP_PD_MASK       0x10    // Page contains VM PD page
//#define PAGESTAMP_PTE_MASK      0x20    // Page contains VM PTE page
#define PAGESTAMP_META_MASK     0x40    // Page contains cached code meta data

} TMemory;


#endif //  _MEMORY_H_
