// ThreadMonochrome.cpp : implementation file
//

#include "stdafx.h"
#include "VMgui.h"
#include "UserCodes.h"
#include "WndMonochrome.h"
#include "ThreadMonochrome.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CThreadMonochrome

IMPLEMENT_DYNCREATE(CThreadMonochrome, CWinThread)

CThreadMonochrome::CThreadMonochrome()
{
}

CThreadMonochrome::~CThreadMonochrome()
{
}

BOOL CThreadMonochrome::InitInstance()
{
    m_bAutoDelete = TRUE;
    m_pWndMonochrome = new CWndMonochrome;
    if (!m_pWndMonochrome)
        return FALSE;
    CString csloc;
    csloc.Format(_T("LocFor %dx%d"), GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));

    int x = AfxGetApp()->GetProfileInt(csloc, _T("Monochrome X"), 0);
    int y = AfxGetApp()->GetProfileInt(csloc, _T("Monochrome Y"), 0);
    int w = AfxGetApp()->GetProfileInt(csloc, _T("Monochrome W"), 0);
    int h = AfxGetApp()->GetProfileInt(csloc, _T("Monochrome H"), 0);

    if (w && h)
        m_pWndMonochrome->SetWindowPos(NULL, x, y, w, h, SWP_NOZORDER);
    m_pWndMonochrome->ShowWindow(SW_SHOW);
    return TRUE;
}

int CThreadMonochrome::ExitInstance()
{
    if (m_pWndMonochrome)
    {
        RECT wndR;
        m_pWndMonochrome->GetWindowRect(&wndR);
        CString csloc;
        csloc.Format(_T("LocFor %dx%d"), GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
        AfxGetApp()->WriteProfileInt(csloc, _T("Monochrome X"), wndR.left);
        AfxGetApp()->WriteProfileInt(csloc, _T("Monochrome Y"), wndR.top);
        AfxGetApp()->WriteProfileInt(csloc, _T("Monochrome W"), wndR.right - wndR.left);
        AfxGetApp()->WriteProfileInt(csloc, _T("Monochrome H"), wndR.bottom - wndR.top);

        m_pWndMonochrome->DestroyWindow();
        m_pWndMonochrome = NULL;
    }
    return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(CThreadMonochrome, CWinThread)
    //{{AFX_MSG_MAP(CThreadMonochrome)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
    ON_MESSAGE(WM_USER, OnUserMessage)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CThreadMonochrome message handlers

LRESULT CThreadMonochrome::OnUserMessage(WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
    case USER_EXIT:         // terminate
        ExitInstance();
        AfxEndThread(0, TRUE);
        break;
    case USER_SHOW_WINDOW:  // show window and bring to top
        m_pWndMonochrome->SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);
        break;
    case USER_SUSPEND:              // stop using timer updates
        m_pWndMonochrome->StopTimer();
        break;
    case USER_RESUME:               // draw resume using timer area
        m_pWndMonochrome->RestartTimer();
        break;
    default:
        break;
    }
    return NOERROR;
}

