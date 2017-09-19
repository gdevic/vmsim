// SettingsVM.cpp : implementation file
//

#include "stdafx.h"
#include "vmgui.h"
#include "SettingsVM.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSettingsVM

IMPLEMENT_DYNAMIC(CSettingsVM, CPropertySheet)

CSettingsVM::CSettingsVM(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
    :CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
    SettingsVM();
}

CSettingsVM::CSettingsVM(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
    :CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
    SettingsVM();
}

CSettingsVM::~CSettingsVM()
{
    delete m_tabFloppy[0];
    delete m_tabFloppy[1];
    delete m_tabIde[0];
    delete m_tabIde[1];
    delete m_tabIde[2];
    delete m_tabIde[3];
}


BEGIN_MESSAGE_MAP(CSettingsVM, CPropertySheet)
    //{{AFX_MSG_MAP(CSettingsVM)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////

void CSettingsVM::SettingsVM()
{
    // TODO: Add your command handler code here

    m_tabFloppy[0] = new CTabFloppy(0);
    m_tabFloppy[1] = new CTabFloppy(1);
    m_tabIde[0]    = new CTabIde(2);
    m_tabIde[1]    = new CTabIde(3);
    m_tabIde[2]    = new CTabIde(4);
    m_tabIde[3]    = new CTabIde(5);

    AddPage(&m_tabVmm);
    AddPage(&m_tabCmos);
    AddPage(m_tabFloppy[0]);
    AddPage(m_tabFloppy[1]);
    AddPage(m_tabIde[0]);
    AddPage(m_tabIde[1]);
    AddPage(m_tabIde[2]);
    AddPage(m_tabIde[3]);

    m_tabFloppy[0]->m_psp.pszTitle = "Floppy A:";
    m_tabFloppy[0]->m_psp.dwFlags |= PSP_USETITLE;
    m_tabFloppy[1]->m_psp.pszTitle = "Floppy B:";
    m_tabFloppy[1]->m_psp.dwFlags |= PSP_USETITLE;

    m_tabIde[0]->m_psp.pszTitle = "IDE 0:0";
    m_tabIde[0]->m_psp.dwFlags |= PSP_USETITLE;
    m_tabIde[1]->m_psp.pszTitle = "IDE 0:1";
    m_tabIde[1]->m_psp.dwFlags |= PSP_USETITLE;
    m_tabIde[2]->m_psp.pszTitle = "IDE 1:0";
    m_tabIde[2]->m_psp.dwFlags |= PSP_USETITLE;
    m_tabIde[3]->m_psp.pszTitle = "IDE 1:1";
    m_tabIde[3]->m_psp.dwFlags |= PSP_USETITLE;

    // Dont stack the tabs

    EnableStackedTabs(FALSE);
}



