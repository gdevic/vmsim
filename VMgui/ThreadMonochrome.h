#if !defined(AFX_THREADMONOCHROME_H__C55ED231_AAE2_4578_9CEB_C9B8116AE6D0__INCLUDED_)
#define AFX_THREADMONOCHROME_H__C55ED231_AAE2_4578_9CEB_C9B8116AE6D0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ThreadMonochrome.h : header file
//


class CWndMonochrome;

/////////////////////////////////////////////////////////////////////////////
// CThreadMonochrome thread

class CThreadMonochrome : public CWinThread
{
    DECLARE_DYNCREATE(CThreadMonochrome)
protected:
    CThreadMonochrome();           // protected constructor used by dynamic creation

// Attributes
public:
    CWndMonochrome      *m_pWndMonochrome;

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CThreadMonochrome)
    public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();
    //}}AFX_VIRTUAL

// Implementation
protected:
    virtual ~CThreadMonochrome();

    // Generated message map functions
    //{{AFX_MSG(CThreadMonochrome)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG
    afx_msg LRESULT OnUserMessage(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_THREADMONOCHROME_H__C55ED231_AAE2_4578_9CEB_C9B8116AE6D0__INCLUDED_)
