#if !defined(AFX_TABIDE_H__A8620034_2103_4D18_99B7_7226BC44F571__INCLUDED_)
#define AFX_TABIDE_H__A8620034_2103_4D18_99B7_7226BC44F571__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TabIde.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTabIde dialog

class CTabIde : public CPropertyPage
{
    DECLARE_DYNCREATE(CTabIde)

// Construction
public:
    CString m_Capacity;
    BOOL m_Virtual;
    CTabIde( UINT nIndex );
    UINT DiskID;
    CTabIde();
    ~CTabIde();

// Dialog Data
    //{{AFX_DATA(CTabIde)
    enum { IDD = IDD_TAB_IDE };
    CString m_FileName;
    BOOL    m_Enabled;
    BOOL    m_Undoable;
    DWORD   m_Sectors;
    DWORD   m_Heads;
    DWORD   m_Tracks;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CTabIde)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CTabIde)
    virtual BOOL OnInitDialog();
    afx_msg void OnBrowse();
    afx_msg void OnCapacity();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TABIDE_H__A8620034_2103_4D18_99B7_7226BC44F571__INCLUDED_)
