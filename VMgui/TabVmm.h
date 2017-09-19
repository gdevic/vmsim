#if !defined(AFX_TABVMM_H__87600E04_3588_4855_8748_83B32D8895FF__INCLUDED_)
#define AFX_TABVMM_H__87600E04_3588_4855_8748_83B32D8895FF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TabVmm.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTabVmm dialog

class CTabVmm : public CPropertyPage
{
    DECLARE_DYNCREATE(CTabVmm)

// Construction
public:
    CString m_HostFreePhysMem;
    CString m_HostPhysMem;
    CTabVmm();
    ~CTabVmm();

// Dialog Data
    //{{AFX_DATA(CTabVmm)
    enum { IDD = IDD_TAB_VMM };
    CString m_FileNameCMOS;
    DWORD   m_PhysMem;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CTabVmm)
    public:
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CTabVmm)
    afx_msg void OnBrowse();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TABVMM_H__87600E04_3588_4855_8748_83B32D8895FF__INCLUDED_)
