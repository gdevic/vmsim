#if !defined(AFX_THREADVGA_H__17B2AFC4_9981_41EE_BE9B_331ADD3F6EAB__INCLUDED_)
#define AFX_THREADVGA_H__17B2AFC4_9981_41EE_BE9B_331ADD3F6EAB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ThreadVGA.h : header file
//

#define VGA_TIMER_REPEAT    1       // non-zero value
#define VGA_TIME_TO_REPEAT  200     // ms

#define BPP 8

/////////////////////////////////////////////////////////////////////////////
// CThreadVGA thread

class CThreadVGA : public CWinThread
{
    DECLARE_DYNCREATE(CThreadVGA)
protected:
    CThreadVGA();           // protected constructor used by dynamic creation

// Attributes
public:
    UINT m_timerIdVGAdraw;
    CWnd *m_pClientWnd;


// Operations
public:
    void DrawClientArea();

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CThreadVGA)
    public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();
    //}}AFX_VIRTUAL

// Implementation
protected:
    virtual ~CThreadVGA();

    // Generated message map functions
    //{{AFX_MSG(CThreadVGA)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG
    afx_msg LRESULT OnUserMessage(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
void CALLBACK TimerForVGAdraw(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2);

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_THREADVGA_H__17B2AFC4_9981_41EE_BE9B_331ADD3F6EAB__INCLUDED_)
