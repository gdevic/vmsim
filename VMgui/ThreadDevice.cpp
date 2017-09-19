// ThreadDevice.cpp : implementation file
//

#include "stdafx.h"
#include "VMgui.h"

#include <winioctl.h>
#include "Ioctl.h"
#include "vm.h"
#include "UserCodes.h"

#include "Simulator.h"
#include "ThreadDevice.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CThreadDevice

IMPLEMENT_DYNCREATE(CThreadDevice, CWinThread)

CThreadDevice::CThreadDevice()
{
}

CThreadDevice::~CThreadDevice()
{
}

BOOL CThreadDevice::InitInstance()
{
    // TODO:  perform and per-thread initialization here
    m_bAutoDelete = TRUE;
    return TRUE;
}

int CThreadDevice::ExitInstance()
{
    // TODO:  perform any per-thread cleanup here
    return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(CThreadDevice, CWinThread)
    //{{AFX_MSG_MAP(CThreadDevice)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
    ON_MESSAGE(WM_USER, OnUserMessage)
    ON_MESSAGE(WM_USER_KEY, OnUserKey)
    ON_MESSAGE(WM_USER_MOUSE_MOVE, OnUserMouseMove)
    ON_MESSAGE(WM_USER_LEFT_BTN_DN, OnUserLeftBtnDn)
    ON_MESSAGE(WM_USER_LEFT_BTN_UP, OnUserLeftBtnUp)
    ON_MESSAGE(WM_USER_LEFT_BTN_DBL, OnUserLeftBtnDbl)
    ON_MESSAGE(WM_USER_RIGHT_BTN_DN, OnUserRightBtnDn)
    ON_MESSAGE(WM_USER_RIGHT_BTN_UP, OnUserRightBtnUp)
    ON_MESSAGE(WM_USER_RIGHT_BTN_DBL, OnUserRightBtnDbl)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CThreadDevice message handlers

LRESULT CThreadDevice::OnUserMessage(WPARAM wParam, LPARAM lParam)
{
    OVERLAPPED overlap = { 0 };
    switch (wParam)
    {
    case USER_EXIT:         // terminate
        AfxEndThread(0, TRUE);
        break;
    case USER_CTRL_ALT_DEL:     // send ctl_alt_del to sim
        DWORD nb;
        VMCONTROL vmCtrl;
        vmCtrl.type = CtlAltDelVM;
        vmCtrl.flags = 0;
        DeviceIoControl( pTheApp->m_pSimulator->sys_handle, VMSIM_Control,
            &vmCtrl, sizeof(VMCONTROL),
            NULL, 0,
            &nb, &overlap );
        break;
    default:
        break;
    }
    return NOERROR;
}

// We get user input when the focus has been grabbed. These
// messages are posted by view.
//
// This function gets a message when a key has been depressed
// as well released.  When we are releasing a focus (CTL+ALT+ESC),
// we get additional message with wParm=lParm=0, and we stuff
// break codes into the VM.
//
// TODO: (Low priority) Keyboard repeat rate

LRESULT CThreadDevice::OnUserKey(WPARAM wParam, LPARAM lParam)
{
    static BOOL fControlPressed = FALSE;
    static BOOL fAltPressed = FALSE;
    DWORD nb;
    BYTE bCode;
    BOOL fPressed;
    OVERLAPPED overlap = { 0 };
    VMINPUTDEVICE vmInput;

    bCode = lParam & 0x7F;
    fPressed = (lParam & 0x8000)==0;

    vmInput.type = InKeyPress;
    vmInput.flags = 0;
    vmInput.scan_code = 0;

    // If we got the message to release the VM focus,
    // we may need to send some break codes

    if( (lParam==0) && (wParam==0) )
    {
        if(fControlPressed==TRUE)
            vmInput.scan_code = 0x1D | 0x80,
            fControlPressed = FALSE;

        if(fAltPressed==TRUE)
        {
            vmInput.scan_code <<= 8;
            vmInput.scan_code = 0x38 | 0x80;
            fAltPressed = FALSE;
        }

        DeviceIoControl( pTheApp->m_pSimulator->sys_handle, VMSIM_InputDevice,
            &vmInput, sizeof(VMINPUTDEVICE),
            NULL, 0,
            &nb, &overlap );

        return NOERROR;
    }

    // Take care of some buffering to remember the state of
    // Ctl/Alt/Esc (we assume Ctl/Alt + Esc the latest will
    // never reach us)

    if( bCode==0x1D )   fControlPressed = fPressed;
    else
        if( bCode==0x38 )   fAltPressed = fPressed;

    // MFII (Multifunction) keyboard layout is supported
    // (not XT or AT).

    // The format of lParam is:
    // 0:7      Scan code
    // 8        Extended key
    // 9:12     Mask out
    // 13       1 if ALT is being pressed down
    // 14       Previous state of a key (0 if key is up)
    // 15       1 if released, 0 if depressed

    vmInput.scan_code = bCode;

    // Set bit 7 if the key was released

    if(fPressed==FALSE)
        vmInput.scan_code |= 0x80;

    if( lParam & 0x100 )
    {
        // Make room for the prefix code
        vmInput.scan_code <<= 8;

        switch( bCode )
        {
        case 0x45:          // Pause key is prefixed by E1
            vmInput.scan_code |= 0xE1;
            break;

        // All other extended keys (numeric etc.) have prefix code E0
        default:
            vmInput.scan_code |= 0xE0;
        }
    }

    DeviceIoControl( pTheApp->m_pSimulator->sys_handle, VMSIM_InputDevice,
        &vmInput, sizeof(VMINPUTDEVICE),
        NULL, 0,
        &nb, &overlap );

    return NOERROR;
}

// wParam is nFlags, lParam is (x << 16) | y
LRESULT CThreadDevice::OnUserMouseMove(WPARAM wParam, LPARAM lParam)
{
    return NOERROR;
}

// wParam is nFlags, lParam is (x << 16) | y
LRESULT CThreadDevice::OnUserLeftBtnDn(WPARAM wParam, LPARAM lParam)
{
    return NOERROR;
}

// wParam is nFlags, lParam is (x << 16) | y
LRESULT CThreadDevice::OnUserLeftBtnUp(WPARAM wParam, LPARAM lParam)
{
    return NOERROR;
}

// wParam is nFlags, lParam is (x << 16) | y
LRESULT CThreadDevice::OnUserLeftBtnDbl(WPARAM wParam, LPARAM lParam)
{
    return NOERROR;
}

// wParam is nFlags, lParam is (x << 16) | y
LRESULT CThreadDevice::OnUserRightBtnDn(WPARAM wParam, LPARAM lParam)
{
    return NOERROR;
}

// wParam is nFlags, lParam is (x << 16) | y
LRESULT CThreadDevice::OnUserRightBtnUp(WPARAM wParam, LPARAM lParam)
{
    return NOERROR;
}

// wParam is nFlags, lParam is (x << 16) | y
LRESULT CThreadDevice::OnUserRightBtnDbl(WPARAM wParam, LPARAM lParam)
{
    return NOERROR;
}

