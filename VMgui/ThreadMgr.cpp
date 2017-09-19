// ThreadMgr.cpp : implementation file
//

#include "stdafx.h"
#include "VMgui.h"

#include "Ioctl.h"
#include "UserCodes.h"
#include "ThreadVGA.h"
#include "ThreadMonochrome.h"
#include "ThreadDevice.h"
#include "ThreadMessages.h"
#include "ThreadMgr.h"
#include "Simulator.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CThreadMgr

IMPLEMENT_DYNCREATE(CThreadMgr, CWinThread)

CThreadMgr::CThreadMgr()
{
}

CThreadMgr::~CThreadMgr()
{
}

BOOL CThreadMgr::InitInstance()
{
    m_bAutoDelete = TRUE;
    m_pThreadMonochrome = (CThreadMonochrome    *)AfxBeginThread(RUNTIME_CLASS(CThreadMonochrome    ));
    m_pThreadVGA        = (CThreadVGA           *)AfxBeginThread(RUNTIME_CLASS(CThreadVGA           ));
    m_pThreadSimMonitor = AfxBeginThread(SimMonitor, LPVOID(pTheApp->m_pSimulator) );
    m_pThreadDevice     = (CThreadDevice        *)AfxBeginThread(RUNTIME_CLASS(CThreadDevice        ));
    m_pThreadRun        = AfxBeginThread(RunVM, LPVOID(pTheApp->m_pSimulator) );
    // optional parameters for AfxBeginThread
    //, int nPriority = THREAD_PRIORITY_NORMAL, UINT nStackSize = 0, DWORD dwCreateFlags = 0, LPSECURITY_ATTRIBUTES lpSecurityAttrs = NULL );
    return TRUE;
}

int CThreadMgr::ExitInstance()
{
    return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(CThreadMgr, CWinThread)
    //{{AFX_MSG_MAP(CThreadMgr)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
    ON_MESSAGE(WM_USER, OnUserMessage)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CThreadMgr message handlers

LRESULT CThreadMgr::OnUserMessage(WPARAM wParam, LPARAM lParam)
{
    //      PostThreadMessage(m_pMonoWindow->m_nThreadID, WM_USER, 1, 1);
    switch (wParam)
    {
    case USER_EXIT:         // terminate
        pTheApp->m_pSimulator->StopVM();        // this closes run and simMonitor
        m_pThreadDevice->PostThreadMessage(WM_USER, USER_EXIT, 0);
        m_pThreadVGA->PostThreadMessage(WM_USER, USER_EXIT, 0);
        m_pThreadMonochrome->PostThreadMessage(WM_USER, USER_EXIT, 0);
        AfxEndThread(0, TRUE);
        break;

    case USER_SHOW_WINDOW:      // show window and bring to top
        break;
    default:
        break;
    }
    return NOERROR;
}




