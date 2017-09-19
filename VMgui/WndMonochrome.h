#if !defined(AFX_WNDMONOCHROME_H__92C1FD14_CDD7_4570_BB83_A6359EF63D40__INCLUDED_)
#define AFX_WNDMONOCHROME_H__92C1FD14_CDD7_4570_BB83_A6359EF63D40__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WndMonochrome.h : header file
//

#define MONO_TIMER_REPEAT   1       // non-zero value
#define MONO_TIME_TO_REPEAT 200     // ms

/////////////////////////////////////////////////////////////////////////////
// CWndMonochrome dialog

class CWndMonochrome : public CDialog
{
// Construction
public:
    CWndMonochrome(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CWndMonochrome)
    enum { IDD = IDD_WND_MONOCHROME };
    CListBox    m_mono_list;
    //}}AFX_DATA

    UINT m_timerIdMonodraw;
    void StopTimer();
    void RestartTimer();

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CWndMonochrome)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CWndMonochrome)
    afx_msg void OnClose();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnTimer(UINT nIDEvent);
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WNDMONOCHROME_H__92C1FD14_CDD7_4570_BB83_A6359EF63D40__INCLUDED_)
