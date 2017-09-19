/******************************************************************************
*                                                                             *
*   Module:     Scanner.h                                                     *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       5/1/2000                                                      *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

    This header file contains defines for the instruction scanner (but not
    insruction decode - look DisassemblerDefines.h)

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 5/1/2000   Original                                             Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Important Defines                                                         *
******************************************************************************/
#ifndef _SCANNER_H_
#define _SCANNER_H_

/******************************************************************************
*                                                                             *
*   Include Files                                                             *
*                                                                             *
******************************************************************************/

/******************************************************************************
*                                                                             *
*   Global Defines, Variables and Macros                                      *
*                                                                             *
******************************************************************************/

// Scanner enums: 4 bits wide

#define SCAN_NATIVE         0x0     // Native instruction are default 0
#define SCAN_JUMP           0x1     // Evaluate new path
#define SCAN_COND_JUMP      0x2     // Evaluate both paths
#define SCAN_TERMINATING    0x3     // Terminating instruction needs virtualization
#define SCAN_TERM_PMODE     0x4     // Terminating instruction in protected mode only
#define SCAN_SINGLE_STEP    0x5     // Single-step instruction

// Define values stored in meta pages (bits [7:4])

#define META_NATIVE         0x0     // Native instruction are default 0
#define META_UNDEF          0x1     // Undefined/illegal instruction
#define META_TERMINATING    0x2     // Terminating instruction
#define META_SINGLE_STEP    0x3     // Execute natively single step

/******************************************************************************
*                                                                             *
*   External Functions                                                        *
*                                                                             *
******************************************************************************/

extern BYTE VmGetNextBYTE();
extern WORD VmGetNextWORD();
extern DWORD VmGetNextDWORD();
extern DWORD DecodeMemoryPointer( DWORD *pFlags, BYTE bOpcode );
extern WORD DecodeEw( DWORD *pFlags, BYTE bOpcode );


#endif //  _SCANNER_H_
