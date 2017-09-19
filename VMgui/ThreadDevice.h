#if !defined(AFX_THREADDEVICE_H__4353AF41_A082_48CE_9D0E_56F2E11ADA18__INCLUDED_)
#define AFX_THREADDEVICE_H__4353AF41_A082_48CE_9D0E_56F2E11ADA18__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ThreadDevice.h : header file
//



/////////////////////////////////////////////////////////////////////////////
// CThreadDevice thread

class CThreadDevice : public CWinThread
{
    DECLARE_DYNCREATE(CThreadDevice)
protected:
    CThreadDevice();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CThreadDevice)
    public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();
    //}}AFX_VIRTUAL

// Implementation
protected:
    virtual ~CThreadDevice();

    // Generated message map functions
    //{{AFX_MSG(CThreadDevice)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG
    afx_msg LRESULT OnUserMessage(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnUserKey(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnUserMouseMove(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnUserLeftBtnDn(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnUserLeftBtnUp(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnUserLeftBtnDbl(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnUserRightBtnDn(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnUserRightBtnUp(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnUserRightBtnDbl(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_THREADDEVICE_H__4353AF41_A082_48CE_9D0E_56F2E11ADA18__INCLUDED_)
