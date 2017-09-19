#if !defined(AFX_TABCMOS_H__55A56A63_F090_4ABB_8A30_2CF5F2BB45AA__INCLUDED_)
#define AFX_TABCMOS_H__55A56A63_F090_4ABB_8A30_2CF5F2BB45AA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TabCmos.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTabCmos dialog

class CTabCmos : public CPropertyPage
{
	DECLARE_DYNCREATE(CTabCmos)

// Construction
public:
	CTabCmos();
	~CTabCmos();

// Dialog Data
	//{{AFX_DATA(CTabCmos)
	enum { IDD = IDD_TAB_CMOS };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CTabCmos)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CTabCmos)
	afx_msg void OnClickTree(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TABCMOS_H__55A56A63_F090_4ABB_8A30_2CF5F2BB45AA__INCLUDED_)
