// TabFloppy.cpp : implementation file
//

#include "stdafx.h"
#include "vmgui.h"
#include "TabFloppy.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTabFloppy property page

IMPLEMENT_DYNCREATE(CTabFloppy, CPropertyPage)

CTabFloppy::CTabFloppy() : CPropertyPage(CTabFloppy::IDD)
{
    //{{AFX_DATA_INIT(CTabFloppy)
    m_FileName = _T("");
    m_Enabled = FALSE;
    m_Undoable = FALSE;
    m_Virtual = FALSE;
    //}}AFX_DATA_INIT
}

CTabFloppy::~CTabFloppy()
{ 
}

void CTabFloppy::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CTabFloppy)
    DDX_Text(pDX, IDC_FILENAME, m_FileName);
    DDV_MaxChars(pDX, m_FileName, 128);
    DDX_Check(pDX, IDC_ENABLED, m_Enabled);
    DDX_Check(pDX, IDC_UNDOABLE, m_Undoable);
    DDX_Check(pDX, IDC_VIRTUAL, m_Virtual);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTabFloppy, CPropertyPage)
    //{{AFX_MSG_MAP(CTabFloppy)
    ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabFloppy message handlers

CTabFloppy::CTabFloppy(UINT nIndex) : CPropertyPage(CTabFloppy::IDD)
{
    m_Enabled  = (pTheApp->m_VMConfig.Disk[nIndex].Flags & DSK_ENABLED)==0? FALSE : TRUE;
    m_Undoable = (pTheApp->m_VMConfig.Disk[nIndex].Flags & DSK_WRITECACHE)==0? FALSE : TRUE;
    m_Virtual  = (pTheApp->m_VMConfig.Disk[nIndex].Flags & DSK_VIRTUAL)==0? FALSE : TRUE;

    m_FileName = pTheApp->m_VMConfig.Disk[nIndex].PathToFile;
}



void CTabFloppy::OnBrowse()
{
    // TODO: Add your control notification handler code here

    CFileDialog File(TRUE,                      // File open dialog
        "dsk",                                  // Default extension
        m_FileName,                             // Default file name
        OFN_CREATEPROMPT | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
        _T("VM Disk files (*.dsk)|*.dsk|All files (*.*)|*.*||") );

    File.m_ofn.lpstrTitle = "Select a virtual file";

    if( File.DoModal()==IDOK )
    {
        m_FileName = File.GetPathName();
        CEdit * pEdit = static_cast<CEdit*>(GetDlgItem(IDC_FILENAME));
        pEdit->SetWindowText(m_FileName);
    }
}

