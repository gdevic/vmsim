#if !defined(AFX_SETTINGSMACHINE_H__9A3AA5C4_8A5D_4678_BCB4_CBD67272AFE4__INCLUDED_)
#define AFX_SETTINGSMACHINE_H__9A3AA5C4_8A5D_4678_BCB4_CBD67272AFE4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SettingsMachine.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSettingsMachine dialog

class CSettingsMachine : public CDialog
{
// Construction
public:
    CSettingsMachine(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CSettingsMachine)
    enum { IDD = IDD_SETTINGS_MACHINE };
    UINT    m_phys_mem;
    CString m_HostFreePhysMem;
    CString m_HostPhysMem;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CSettingsMachine)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CSettingsMachine)
    virtual void OnOK();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SETTINGSMACHINE_H__9A3AA5C4_8A5D_4678_BCB4_CBD67272AFE4__INCLUDED_)
