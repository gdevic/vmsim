/******************************************************************************
*                                                                             *
*   Module:     Debug.h                                                       *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       5/4/2000                                                      *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This is a header file containing the debugging and printing
        facilities

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 5/4/2000   Original                                             Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Important Defines                                                         *
******************************************************************************/
#ifndef _DEBUG_H_
#define _DEBUG_H_

/******************************************************************************
*                                                                             *
*   Global Defines, Variables and Macros                                      *
*                                                                             *
******************************************************************************/
#if DBG

// There are 2 kinds of breakpoints: INT 1 and INT 3
// Throughout the code use these macros to place them:

#define BreakpointA         _asm int 1
#define BreakpointB         _asm int 3
#define Breakpoint          BreakpointB


extern int fSingleStep;
extern int fSilent;

#define DbgPrint(arg) DbgPrint arg

/*-----------------------------------------------------------------------------
    There are two message mechanisms: trace and warning and they are
    independent of each other.

    Trace is enabled by setting a non-zero value into pMonitor->fEnableTrace
    and will print a major function that is entered as well as the
    arguments of a call.

    Warning messages are enabled by setting a non-zero value into
    pMonitor->fEnableWarnings.

    Warnings are generally used to print a message in the case of a non-fatal
    irregularity in execution, or where a functionality is not supported, but
    is not critical.

    For critical errors where the execution would be mostly undefined afterwards
    use ASSERT macro.
------------------------------------------------------------------------------*/

#define TRACE(strings)                                      \
{                                                           \
    if(!pVM || pVM->fEnableTrace)                           \
    {                                                       \
        DebugPrint2("%s,%d: ", __FILE__, __LINE__);         \
        DebugPrint2##strings;                               \
        DebugPrint2("\n");                                  \
    }                                                       \
}

#define WARNING(strings)                                    \
{                                                           \
    if(!pVM || pVM->fEnableWarnings)                        \
    {                                                       \
        DebugPrint2("Warning: %s,%d: ", __FILE__, __LINE__);\
        DebugPrint2##strings;                               \
        DebugPrint2("\n");                                  \
    }                                                       \
}

#define INFO(strings)                                       \
{                                                           \
    if(!pVM || pVM->fEnableWarnings)                        \
    {                                                       \
        DebugPrint2##strings;                               \
        DebugPrint2("\n");                                  \
    }                                                       \
}

#define ERROR(strings)                                      \
{                                                           \
    DebugPrint2("Error: %s,%d: ", __FILE__, __LINE__);      \
    DebugPrint2##strings;                                   \
    DebugPrint2("\n");                                      \
    ASSERT(0);                                              \
}

#else // !DBG =================================================================

#define BreakpointA
#define BreakpointB
#define Breakpoint

#define DbgPrint(arg)
#define DebugPrint2(arg)

#define INFO(args)            NULL
#define TRACE(args)           NULL
#define WARNING(str)          NULL
#define ERROR(str)            NULL

#endif  // DBG

/******************************************************************************
*                                                                             *
*   External Functions                                                        *
*                                                                             *
******************************************************************************/
#if DBG

extern void DebugPrint2(char * szFormat, ...);

#endif  // DBG

#endif  //  _DEBUG_H_
