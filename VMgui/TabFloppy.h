#if !defined(AFX_TABFLOPPY_H__807E1CD0_64FF_439C_BC1F_840FCC6D62EE__INCLUDED_)
#define AFX_TABFLOPPY_H__807E1CD0_64FF_439C_BC1F_840FCC6D62EE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TabFloppy.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTabFloppy dialog

class CTabFloppy : public CPropertyPage
{
    DECLARE_DYNCREATE(CTabFloppy)

// Construction
public:
    CTabFloppy();
    ~CTabFloppy();
    CTabFloppy(UINT nIndex);

// Dialog Data
    //{{AFX_DATA(CTabFloppy)
    enum { IDD = IDD_TAB_FLOPPY };
    CString m_FileName;
    BOOL    m_Enabled;
    BOOL    m_Undoable;
    BOOL    m_Virtual;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CTabFloppy)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CTabFloppy)
    afx_msg void OnBrowse();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TABFLOPPY_H__807E1CD0_64FF_439C_BC1F_840FCC6D62EE__INCLUDED_)
