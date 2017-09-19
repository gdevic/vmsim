// WndMonochrome.cpp : implementation file
//

#include "stdafx.h"
#include "VMgui.h"
#include "WndMonochrome.h"
#include "vm.h"
#include "ioctl.h"
#include "simulator.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWndMonochrome dialog


CWndMonochrome::CWndMonochrome(CWnd* pParent /*=NULL*/)
    : CDialog(CWndMonochrome::IDD, pParent)
{
    //{{AFX_DATA_INIT(CWndMonochrome)
    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    m_timerIdMonodraw = 0;
    Create(CWndMonochrome::IDD, pParent);
}


void CWndMonochrome::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CWndMonochrome)
    DDX_Control(pDX, IDC_MONOCHROME_LIST, m_mono_list);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWndMonochrome, CDialog)
    //{{AFX_MSG_MAP(CWndMonochrome)
    ON_WM_CLOSE()
    ON_WM_SIZE()
    ON_WM_TIMER()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWndMonochrome message handlers

void CWndMonochrome::PostNcDestroy()
{
    delete this;
}

void CWndMonochrome::OnClose()
{
    ShowWindow(SW_HIDE);
}

void CWndMonochrome::OnSize(UINT nType, int cx, int cy)
{
    CDialog::OnSize(nType, cx, cy);

    m_mono_list.SetWindowPos(NULL, 0, 0, cx-20, cy-20, SWP_NOMOVE | SWP_NOZORDER);

}

void CWndMonochrome::OnTimer(UINT nIDEvent)
{
    PBYTE pMDA = (PBYTE)&pTheApp->m_pSimulator->pVM->MdaTextBuffer;
    m_mono_list.SetRedraw(FALSE);
    m_mono_list.ResetContent();
    for (int r = 0; r < 25; r++)
    {
        char str[100];
        char *pch = str;
        for (int c = 0; c < 80; c++)
        {
            BYTE attr;
            attr = *pMDA++;
            *pch++ = *pMDA++;
        }
        *pch = '\0';
        m_mono_list.AddString(str);
    }
    m_mono_list.SetRedraw(TRUE);
    m_mono_list.Invalidate(FALSE);      // cause the redraw

    CDialog::OnTimer(nIDEvent);
}

BOOL CWndMonochrome::OnInitDialog()
{
    CDialog::OnInitDialog();

        m_timerIdMonodraw = SetTimer(MONO_TIMER_REPEAT, MONO_TIME_TO_REPEAT, 0);
    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void CWndMonochrome::StopTimer()
{
    if (m_timerIdMonodraw)
    {
        KillTimer(m_timerIdMonodraw);
        m_timerIdMonodraw = 0;
    }
}

void CWndMonochrome::RestartTimer()
{
    if (!m_timerIdMonodraw)
    {
        m_timerIdMonodraw = SetTimer(MONO_TIMER_REPEAT, MONO_TIME_TO_REPEAT, 0);
    }
}
