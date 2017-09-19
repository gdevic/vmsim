// WndDbgMessages.cpp : implementation file
//

#include "stdafx.h"
#include "VMgui.h"
#include "WndDbgMessages.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWndDbgMessages dialog


CWndDbgMessages::CWndDbgMessages(CWnd* pParent /*=NULL*/)
    : CDialog(CWndDbgMessages::IDD, pParent)
{
    //{{AFX_DATA_INIT(CWndDbgMessages)
    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    Create(CWndDbgMessages::IDD, pParent);
}


void CWndDbgMessages::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CWndDbgMessages)
    DDX_Control(pDX, IDC_MESSAGE_LIST, m_dbg_messages);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWndDbgMessages, CDialog)
    //{{AFX_MSG_MAP(CWndDbgMessages)
    ON_WM_CLOSE()
    ON_WM_SIZE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWndDbgMessages message handlers

void CWndDbgMessages::PostNcDestroy()
{
    delete this;
}

void CWndDbgMessages::OnClose()
{
    ShowWindow(SW_HIDE);
}

void CWndDbgMessages::OnSize(UINT nType, int cx, int cy)
{
    CDialog::OnSize(nType, cx, cy);

    m_dbg_messages.SetWindowPos(NULL, 0, 0, cx-20, cy-20, SWP_NOMOVE | SWP_NOZORDER);

}

