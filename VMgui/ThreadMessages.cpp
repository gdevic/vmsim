// ThreadMessages.cpp : implementation file
//

#include "stdafx.h"
#include "VMgui.h"
#include "UserCodes.h"
#include "WndDbgMessages.h"
#include "ThreadMessages.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CThreadMessages

IMPLEMENT_DYNCREATE(CThreadMessages, CWinThread)

CThreadMessages::CThreadMessages()
{
}

CThreadMessages::~CThreadMessages()
{
}

BOOL CThreadMessages::InitInstance()
{
    m_bAutoDelete = TRUE;
    m_pWndDbgMessages = new CWndDbgMessages;
    if (!m_pWndDbgMessages)
        return FALSE;
    CString csloc;
    csloc.Format(_T("LocFor %dx%d"), GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));

    int x = AfxGetApp()->GetProfileInt(csloc, _T("DbgMssg X"), 0);
    int y = AfxGetApp()->GetProfileInt(csloc, _T("DbgMssg Y"), 0);
    int w = AfxGetApp()->GetProfileInt(csloc, _T("DbgMssg W"), 0);
    int h = AfxGetApp()->GetProfileInt(csloc, _T("DbgMssg H"), 0);

    if (w && h)
        m_pWndDbgMessages->SetWindowPos(NULL, x, y, w, h, SWP_NOZORDER);
    m_pWndDbgMessages->ShowWindow(SW_SHOW);

    return TRUE;
}

int CThreadMessages::ExitInstance()
{
    if (m_pWndDbgMessages)
    {
        RECT wndR;
        m_pWndDbgMessages->GetWindowRect(&wndR);
        CString csloc;
        csloc.Format(_T("LocFor %dx%d"), GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
        AfxGetApp()->WriteProfileInt(csloc, _T("DbgMssg X"), wndR.left);
        AfxGetApp()->WriteProfileInt(csloc, _T("DbgMssg Y"), wndR.top);
        AfxGetApp()->WriteProfileInt(csloc, _T("DbgMssg W"), wndR.right - wndR.left);
        AfxGetApp()->WriteProfileInt(csloc, _T("DbgMssg H"), wndR.bottom - wndR.top);

        m_pWndDbgMessages->DestroyWindow();
        m_pWndDbgMessages = NULL;
    }
    return CWinThread::ExitInstance();
}


BEGIN_MESSAGE_MAP(CThreadMessages, CWinThread)
    //{{AFX_MSG_MAP(CThreadMessages)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
    ON_MESSAGE(WM_USER, OnUserMessage)
    ON_MESSAGE(WM_USER_DBG_MESSAGE, OnUserDbgMessage)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CThreadMessages message handlers

LRESULT CThreadMessages::OnUserMessage(WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
    case USER_EXIT:         // terminate
        ExitInstance();
        AfxEndThread(0, TRUE);
        break;
    case USER_SHOW_WINDOW:  // show window and bring to top
        m_pWndDbgMessages->SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);
        break;
    default:
        break;
    }
    return NOERROR;
}



LRESULT CThreadMessages::OnUserDbgMessage(WPARAM wParam, LPARAM lParam)
{
    DWORD bytes = (wParam & 0xFF);
    DWORD flag = (wParam >> 8);
    PCHAR str = (PCHAR)lParam;
    CString cs = str;

    m_pWndDbgMessages->m_dbg_messages.AddString(cs);
    // Select the last item in the list box.
    int cnt = m_pWndDbgMessages->m_dbg_messages.GetCount();
    if (cnt > 0)
    {
        m_pWndDbgMessages->m_dbg_messages.SetSel(-1, FALSE);
        m_pWndDbgMessages->m_dbg_messages.SetSel(cnt-1, TRUE);
    }
    return NOERROR;
}

