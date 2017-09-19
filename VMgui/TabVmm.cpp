// TabVmm.cpp : implementation file
//

#include "stdafx.h"
#include "vmgui.h"
#include "TabVmm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTabVmm property page

IMPLEMENT_DYNCREATE(CTabVmm, CPropertyPage)

CTabVmm::CTabVmm() : CPropertyPage(CTabVmm::IDD)
{
    //{{AFX_DATA_INIT(CTabVmm)
    m_FileNameCMOS = _T("");
    m_PhysMem = 0;
    //}}AFX_DATA_INIT

    m_FileNameCMOS = pTheApp->m_VMConfig.PathToCMOS;
    m_PhysMem      = pTheApp->m_VMConfig.PhysMem;
}

CTabVmm::~CTabVmm()
{
}

void CTabVmm::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CTabVmm)
    DDX_Text(pDX, IDC_FILENAME, m_FileNameCMOS);
    DDV_MaxChars(pDX, m_FileNameCMOS, 128);
    DDX_Text(pDX, IDC_VRAM, m_PhysMem);
    DDV_MinMaxDWord(pDX, m_PhysMem, 4, 256);
    //}}AFX_DATA_MAP

    pDX->PrepareEditCtrl(IDC_VRAM);
    DDV_MinMaxLong(pDX, m_PhysMem, 4, pTheApp->m_HostFreePhysMem);
}


BEGIN_MESSAGE_MAP(CTabVmm, CPropertyPage)
    //{{AFX_MSG_MAP(CTabVmm)
    ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabVmm message handlers

void CTabVmm::OnBrowse()
{
    // TODO: Add your control notification handler code here

    CFileDialog File(TRUE,                      // File open dialog
        "bin",                                  // Default extension
        m_FileNameCMOS,                         // Default file name
        OFN_CREATEPROMPT | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
        _T("Binary files (*.bin)|*.bin|All files (*.*)|*.*||") );

    File.m_ofn.lpstrTitle = "Select CMOS virtual file";

    if( File.DoModal()==IDOK )
    {
        m_FileNameCMOS = File.GetPathName();
        CEdit * pEdit = static_cast<CEdit*>(GetDlgItem(IDC_FILENAME));
        pEdit->SetWindowText(m_FileNameCMOS);
    }
}


BOOL CTabVmm::OnInitDialog()
{
    CPropertyPage::OnInitDialog();

    // TODO: Add extra initialization here

    // Initialize property page controls
    CStatic * pStatic;

    // Load the class variables

    m_HostPhysMem.Format("Physical memory installed: %d [Mb]", pTheApp->m_HostPhysMem);
    m_HostFreePhysMem.Format("Available RAM size: %d [Mb]", pTheApp->m_HostFreePhysMem);

    // Set the fields of the dialog

    pStatic = static_cast<CStatic*>(GetDlgItem(IDC_SYSTEM_MEMORY));
    pStatic->SetWindowText(m_HostPhysMem);

    pStatic = static_cast<CStatic*>(GetDlgItem(IDC_AVAILABLE));
    pStatic->SetWindowText(m_HostFreePhysMem);


    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}



