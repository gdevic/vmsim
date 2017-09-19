/******************************************************************************
*                                                                             *
*   Module:     Types.h                                                       *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       3/10/2000                                                     *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This header file defines extended data types for C compiler

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 3/10/2000  Original                                             Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Important Defines                                                         *
******************************************************************************/
#ifndef _TYPES_H_
#define _TYPES_H_

/******************************************************************************
*                                                                             *
*   Global Defines, Variables and Macros                                      *
*                                                                             *
******************************************************************************/

#ifndef offsetof
#define offsetof(s,m) (int)&(((s*)0)->m)
#endif

// Allow redefinition of these extended data types as long as they are
// based on the same basic data type
#pragma warning(disable: 4142)

typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;
#ifndef _WINDEF_
typedef unsigned int BOOL;
#endif

// We have to make sure the TRUE is 1, and not some other non-zero value,
// since we use it directly to set some register values

#undef TRUE
#define TRUE    1

#undef FALSE
#define FALSE   0

// Define NOT for boolean operation that will always operate on 1 or 0

#define NOT(fBool)  (fBool ^ 1)

#endif // _TYPES_H_
