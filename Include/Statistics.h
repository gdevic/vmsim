/******************************************************************************
*                                                                             *
*   Module:     Statistics.h                                                  *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       5/17/2000                                                     *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This is a header file containing statistics data and counters

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 5/17/2000  Original                                             Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Important Defines                                                         *
******************************************************************************/
#ifndef _STATS_H_
#define _STATS_H_

/******************************************************************************
*                                                                             *
*   Global Defines, Variables and Macros                                      *
*                                                                             *
******************************************************************************/

typedef struct
{
    DWORD cMainLoop;                    // Main loop counter
    DWORD cExternalInt;                 // Number of external interrupts
    DWORD cCpuInt;                      // Number of CPU interrupts
    DWORD cPushInt;                     // Number of external interrupts pushed into the VM
    DWORD cExtInt[256];                 // Number of external interrupts to a VM

} TStats;


#if INCLUDE_STATS
//-----------------------------------------------------------------------------
// Use these macros to increment statistic data, so the code can be turned
// off for retail build when we dont need it
//-----------------------------------------------------------------------------

#define STATS_INC(field)        if(pVM) (pVM->Stats.##field)++
#define STATS_EXTINT(nInt)      if(pVM) (pVM->Stats.cExtInt[nInt])++

//-----------------------------------------------------------------------------
#else // INCLUDE_STATS==0
//-----------------------------------------------------------------------------

#define STATS_INC(field)
#define STATS_EXTINT(nInt)

//-----------------------------------------------------------------------------
#endif // INCLUDE_STATS
//-----------------------------------------------------------------------------

#endif //  _STATS_H_
