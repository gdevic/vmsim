/******************************************************************************
*                                                                             *
*   Module:     CmosAndRtc.cpp                                                *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       5/30/2000                                                     *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the code for the Ring-3 user support of the
        CMOS and Rtc virtual device.

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 5/30/2000  Original                                             Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Include Files                                                             *
******************************************************************************/
#include "stdafx.h"
#include "VMgui.h"

#include <winioctl.h>
#include "Ioctl.h"
#include "vm.h"

#include "Interface.h"
#include "Simulator.h"

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

#define BCD(x)  ((((x)/10)<<4) | ((x)%10))


/******************************************************************************
*                                                                             *
*   Functions                                                                 *
*                                                                             *
******************************************************************************/

//----------------------------------------------------------------
// This function is part of Ring-3 support for virtual CMOS and
// Rtc device.
//----------------------------------------------------------------
void CSimulator::UpdateClock()
{
    // Initialize Clock
    Clock = CTime::GetCurrentTime();

    // Make sure we dont touch pVM unless we have it
    if( pVM )
    {
        pVM->CMOS[0x00] = BCD( Clock.GetSecond() );
        pVM->CMOS[0x02] = BCD( Clock.GetMinute() );
        pVM->CMOS[0x04] = BCD( Clock.GetHour() );
        pVM->CMOS[0x06] = BCD( Clock.GetDayOfWeek() );
        pVM->CMOS[0x07] = BCD( Clock.GetDay() );
        pVM->CMOS[0x08] = BCD( Clock.GetMonth() );
        pVM->CMOS[0x09] = BCD( Clock.GetYear() % 100 );
        pVM->CMOS[0x32] = BCD( 20 );                    // Century
    }
}



