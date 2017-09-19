#if !defined(AFX_SETTINGSVM_H__B0F93156_1249_4D47_A02F_4F7D48AB6074__INCLUDED_)
#define AFX_SETTINGSVM_H__B0F93156_1249_4D47_A02F_4F7D48AB6074__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SettingsVM.h : header file
//

#include "TabFloppy.h"
#include "TabIde.h"
#include "TabVmm.h"
#include "TabCmos.h"

/////////////////////////////////////////////////////////////////////////////
// CSettingsVM

class CSettingsVM : public CPropertySheet
{
    DECLARE_DYNAMIC(CSettingsVM)

// Construction
public:
    CSettingsVM(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
    CSettingsVM(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:

    CTabVmm     m_tabVmm;
    CTabCmos    m_tabCmos;
    CTabFloppy  *m_tabFloppy[2];
    CTabIde     *m_tabIde[4];

// Operations
public:
    void SettingsVM();
// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CSettingsVM)
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CSettingsVM();

    // Generated message map functions
protected:
    //{{AFX_MSG(CSettingsVM)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SETTINGSVM_H__B0F93156_1249_4D47_A02F_4F7D48AB6074__INCLUDED_)
