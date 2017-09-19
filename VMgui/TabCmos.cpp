// TabCmos.cpp : implementation file
//

#include "stdafx.h"
#include "vmgui.h"
#include "TabCmos.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTabCmos property page

IMPLEMENT_DYNCREATE(CTabCmos, CPropertyPage)

CTabCmos::CTabCmos() : CPropertyPage(CTabCmos::IDD)
{
	//{{AFX_DATA_INIT(CTabCmos)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CTabCmos::~CTabCmos()
{
}

void CTabCmos::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTabCmos)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTabCmos, CPropertyPage)
	//{{AFX_MSG_MAP(CTabCmos)
	ON_NOTIFY(NM_CLICK, IDC_TREE1, OnClickTree)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabCmos message handlers

void CTabCmos::OnClickTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	
	*pResult = 0;
}
