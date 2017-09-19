#if !defined(AFX_THREADMESSAGES_H__ECE4CD4E_3336_4941_9A8D_B40A2FC4346A__INCLUDED_)
#define AFX_THREADMESSAGES_H__ECE4CD4E_3336_4941_9A8D_B40A2FC4346A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ThreadMessages.h : header file
//

class CWndDbgMessages;

/////////////////////////////////////////////////////////////////////////////
// CThreadMessages thread

class CThreadMessages : public CWinThread
{
    DECLARE_DYNCREATE(CThreadMessages)
protected:
    CThreadMessages();           // protected constructor used by dynamic creation

// Attributes
public:
    CWndDbgMessages     *m_pWndDbgMessages;

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CThreadMessages)
    public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();
    //}}AFX_VIRTUAL

// Implementation
protected:
    virtual ~CThreadMessages();

    // Generated message map functions
    //{{AFX_MSG(CThreadMessages)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG
    afx_msg LRESULT OnUserMessage(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnUserDbgMessage(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_THREADMESSAGES_H__ECE4CD4E_3336_4941_9A8D_B40A2FC4346A__INCLUDED_)
