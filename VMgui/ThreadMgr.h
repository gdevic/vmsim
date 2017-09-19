#if !defined(AFX_THREADMGR_H__55BC064B_8476_47B4_A196_E0B840F5FEB8__INCLUDED_)
#define AFX_THREADMGR_H__55BC064B_8476_47B4_A196_E0B840F5FEB8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ThreadMgr.h : header file
//


#define USER_SHOW_WINDOW    1


class CThreadVGA;
class CThreadMonochrome;
class CThreadSimMonitor;
class CThreadDevice;



/////////////////////////////////////////////////////////////////////////////
// CThreadMgr thread

class CThreadMgr : public CWinThread
{
    DECLARE_DYNCREATE(CThreadMgr)
protected:
    CThreadMgr();           // protected constructor used by dynamic creation

// Attributes
public:
    CThreadVGA          *m_pThreadVGA;
    CThreadMonochrome   *m_pThreadMonochrome;
    CWinThread          *m_pThreadRun;
    CWinThread          *m_pThreadSimMonitor;
    CThreadDevice       *m_pThreadDevice;

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CThreadMgr)
    public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();
    //}}AFX_VIRTUAL

// Implementation
protected:
    virtual ~CThreadMgr();

    // Generated message map functions
    //{{AFX_MSG(CThreadMgr)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG
    afx_msg LRESULT OnUserMessage(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_THREADMGR_H__55BC064B_8476_47B4_A196_E0B840F5FEB8__INCLUDED_)
