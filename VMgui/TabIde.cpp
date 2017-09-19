// TabIde.cpp : implementation file
//

#include "stdafx.h"
#include "vmgui.h"
#include "TabIde.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTabIde property page

IMPLEMENT_DYNCREATE(CTabIde, CPropertyPage)

CTabIde::CTabIde() : CPropertyPage(CTabIde::IDD)
{
    //{{AFX_DATA_INIT(CTabIde)
    m_FileName = _T("");
    m_Enabled = FALSE;
    m_Undoable = FALSE;
    m_Sectors = 0;
    m_Heads = 0;
    m_Tracks = 0;
    //}}AFX_DATA_INIT
}

CTabIde::~CTabIde()
{
}

void CTabIde::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CTabIde)
    DDX_Text(pDX, IDC_FILENAME, m_FileName);
    DDV_MaxChars(pDX, m_FileName, 128);
    DDX_Check(pDX, IDC_ENABLED, m_Enabled);
    DDX_Check(pDX, IDC_UNDOABLE, m_Undoable);
    DDX_Text(pDX, IDC_SECTORS, m_Sectors);
    DDX_Text(pDX, IDC_HEADS, m_Heads);
    DDX_Text(pDX, IDC_TRACKS, m_Tracks);
    //}}AFX_DATA_MAP

    if( pDX->m_bSaveAndValidate==TRUE )
    {
        OnCapacity();

        CButton *pEnabled = static_cast<CButton*>(GetDlgItem(IDC_ENABLED));
        if(pEnabled->GetCheck()==BST_CHECKED)
        {
            DDV_MinMaxDWord(pDX, m_Tracks, 1, 65536);
            DDV_MinMaxDWord(pDX, m_Heads, 1, 64);
            DDV_MinMaxDWord(pDX, m_Sectors, 1, 65536);
        }
    }
}


BEGIN_MESSAGE_MAP(CTabIde, CPropertyPage)
    //{{AFX_MSG_MAP(CTabIde)
    ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
    ON_BN_CLICKED(IDC_CAPACITY, OnCapacity)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabIde message handlers

CTabIde::CTabIde(UINT nIndex) : CPropertyPage(CTabIde::IDD)
{
    m_Enabled  = (pTheApp->m_VMConfig.Disk[nIndex].Flags & DSK_ENABLED)==0? FALSE : TRUE;
    m_Undoable = (pTheApp->m_VMConfig.Disk[nIndex].Flags & DSK_WRITECACHE)==0? FALSE : TRUE;

    m_FileName = pTheApp->m_VMConfig.Disk[nIndex].PathToFile;

    m_Tracks   = pTheApp->m_VMConfig.Disk[nIndex].tracks;
    m_Heads    = pTheApp->m_VMConfig.Disk[nIndex].heads;
    m_Sectors  = pTheApp->m_VMConfig.Disk[nIndex].sectors;
}



BOOL CTabIde::OnInitDialog()
{
    CPropertyPage::OnInitDialog();

    // TODO: Add extra initialization here

    // Initialize property page controls
    OnCapacity();

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void CTabIde::OnBrowse()
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

void CTabIde::OnCapacity()
{
    CEdit *pEdit;
    char buffer[32];
    DWORD tracks, heads, sectors;

    pEdit = static_cast<CEdit *>(GetDlgItem(IDC_TRACKS));
    pEdit->GetLine(0, buffer, sizeof(buffer));
    sscanf(buffer, "%d", &tracks);

    pEdit = static_cast<CEdit *>(GetDlgItem(IDC_HEADS));
    pEdit->GetLine(0, buffer, sizeof(buffer));
    sscanf(buffer, "%d", &heads);

    pEdit = static_cast<CEdit *>(GetDlgItem(IDC_SECTORS));
    pEdit->GetLine(0, buffer, sizeof(buffer));
    sscanf(buffer, "%d", &sectors);

    m_Capacity.Format("Disk capacity: %d [Mb]", (tracks * heads * sectors * 512) / (1024 * 1024) );

    // Set the fields of the dialog

    CButton *pButton = static_cast<CButton*>(GetDlgItem(IDC_CAPACITY));
    pButton->SetWindowText(m_Capacity);
}

