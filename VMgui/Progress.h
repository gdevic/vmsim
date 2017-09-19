#if !defined(AFX_PROGRESS_H__A95AFBCE_75D9_4C5D_B523_79F3AFB703EE__INCLUDED_)
#define AFX_PROGRESS_H__A95AFBCE_75D9_4C5D_B523_79F3AFB703EE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Progress.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CProgress dialog

class CProgress : public CDialog
{
// Construction
public:
    void SetRange(int low, int high);
    void SetText(const char *sMessage);
    ~CProgress();
    void SetPos(UINT nPos);
    BOOL Create();
    CProgress(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CProgress)
    enum { IDD = IDD_PROGRESS };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CProgress)
    public:
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CProgress)
        // NOTE: the ClassWizard will add member functions here
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROGRESS_H__A95AFBCE_75D9_4C5D_B523_79F3AFB703EE__INCLUDED_)
