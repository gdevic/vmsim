// Progress.cpp : implementation file
//

#include "stdafx.h"
#include "vmgui.h"
#include "Progress.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProgress dialog


CProgress::CProgress(CWnd* pParent /*=NULL*/)
    : CDialog(CProgress::IDD, pParent)
{
    //{{AFX_DATA_INIT(CProgress)
    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CProgress::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CProgress)
    // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CProgress, CDialog)
    //{{AFX_MSG_MAP(CProgress)
        // NOTE: the ClassWizard will add message map macros here
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProgress message handlers


BOOL CProgress::Create()
{
    return CDialog::Create(IDD_PROGRESS, this);
}

void CProgress::SetPos(UINT nPos)
{
    CProgressCtrl *q = static_cast<CProgressCtrl *>(GetDlgItem(IDC_PROGRESS));

    q->SetPos(nPos);
}

CProgress::~CProgress()
{
    DestroyWindow();
}

void CProgress::SetText(const char *sMessage)
{
    CStatic *t = static_cast<CStatic *>(GetDlgItem(IDC_MESSAGE));
    t->SetWindowText(sMessage);
}

void CProgress::SetRange(int low, int high)
{
    CProgressCtrl *q = static_cast<CProgressCtrl *>(GetDlgItem(IDC_PROGRESS));

    q->SetRange32(low, high);
    q->SetStep(1);
}

