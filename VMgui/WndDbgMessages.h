#if !defined(AFX_WNDDBGMESSAGES_H__5C76C648_0016_4C52_885F_B73D441C8D7D__INCLUDED_)
#define AFX_WNDDBGMESSAGES_H__5C76C648_0016_4C52_885F_B73D441C8D7D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WndDbgMessages.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWndDbgMessages dialog

class CWndDbgMessages : public CDialog
{
// Construction
public:
    CWndDbgMessages(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CWndDbgMessages)
    enum { IDD = IDD_DBG_MESSAGES };
    CListBox    m_dbg_messages;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CWndDbgMessages)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CWndDbgMessages)
    afx_msg void OnClose();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WNDDBGMESSAGES_H__5C76C648_0016_4C52_885F_B73D441C8D7D__INCLUDED_)
