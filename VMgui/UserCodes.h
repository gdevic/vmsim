/******************************************************************************
*                                                                             *
*   Module:     UserCodes.h                                                   *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       5/12/2000                                                     *
*                                                                             *
*   Author:     Garry Amann                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the defines for User Messages

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 5/12/2000  Original                                             Garry Amann *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Include Files                                                             *
******************************************************************************/
#ifndef _USERCODES_H_
#define _USERCODES_H_

/******************************************************************************
*                                                                             *
*   UserCodes                                                                 *
*                                                                             *
******************************************************************************/

// wParam codes for WM_USER
#define USER_EXIT           0       // exit the thread
#define USER_SHOW_WINDOW    1       // show window and bring to top
#define USER_DRAW           2       // draw client area
#define USER_CTRL_ALT_DEL   3       // send ctl_alt_del to sim
#define USER_SUSPEND        4       // stop using timer updates
#define USER_RESUME         5       // resume using timer

#define WM_USER_KEYDOWN         (WM_USER + 1)
#define WM_USER_KEY             (WM_USER + 2)
#define WM_USER_MOUSE_MOVE      (WM_USER + 3)
#define WM_USER_LEFT_BTN_DN     (WM_USER + 4)
#define WM_USER_LEFT_BTN_UP     (WM_USER + 5)
#define WM_USER_LEFT_BTN_DBL    (WM_USER + 6)
#define WM_USER_RIGHT_BTN_DN    (WM_USER + 7)
#define WM_USER_RIGHT_BTN_UP    (WM_USER + 8)
#define WM_USER_RIGHT_BTN_DBL   (WM_USER + 9)
#define WM_USER_DBG_MESSAGE     (WM_USER + 10)


#endif // _USERCODES_H_
