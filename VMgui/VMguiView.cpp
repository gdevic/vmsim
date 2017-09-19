// VMguiView.cpp : implementation of the CVMguiView class
//

#include "stdafx.h"
#include "VMgui.h"

#include "VMguiDoc.h"
#include "VMguiView.h"
#include "UserCodes.h"
#include "ThreadVGA.h"
#include "ThreadDevice.h"
#include "ThreadMgr.h"
#include "ThreadMessages.h"
#include "WndDbgMessages.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVMguiView

IMPLEMENT_DYNCREATE(CVMguiView, CView)

BEGIN_MESSAGE_MAP(CVMguiView, CView)
    //{{AFX_MSG_MAP(CVMguiView)
    ON_WM_KEYDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONDBLCLK()
    ON_WM_LBUTTONUP()
    ON_WM_RBUTTONDOWN()
    ON_WM_RBUTTONUP()
    ON_WM_RBUTTONDBLCLK()
    ON_WM_SIZE()
    ON_WM_KEYUP()
    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    //}}AFX_MSG_MAP
    // Standard printing commands
    ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVMguiView construction/destruction

CVMguiView::CVMguiView()
{
    // TODO: add construction code here

}

CVMguiView::~CVMguiView()
{
}

BOOL CVMguiView::PreCreateWindow(CREATESTRUCT& cs)
{
    // TODO: Modify the Window class or styles here by modifying
    //  the CREATESTRUCT cs

    return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CVMguiView drawing

void CVMguiView::OnDraw(CDC* pDC)
{
    //CVMguiDoc* pDoc = GetDocument();
    //ASSERT_VALID(pDoc);
    // TODO: add draw code for native data here
    if (pTheApp->m_pThreadMgr)
        ((CThreadMgr *)pTheApp->m_pThreadMgr)->m_pThreadVGA->PostThreadMessage(WM_USER, USER_DRAW, 0);  // since this is post, pDC will be invalid
    else
    {
        RECT cRect;
        GetClientRect(&cRect);
        pDC->FillSolidRect(&cRect, RGB(0, 0, 70));
    }

}

/////////////////////////////////////////////////////////////////////////////
// CVMguiView printing

BOOL CVMguiView::OnPreparePrinting(CPrintInfo* pInfo)
{
    // default preparation
    return DoPreparePrinting(pInfo);
}

void CVMguiView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
    // TODO: add extra initialization before printing
}

void CVMguiView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
    // TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CVMguiView diagnostics

#ifdef _DEBUG
void CVMguiView::AssertValid() const
{
    CView::AssertValid();
}

void CVMguiView::Dump(CDumpContext& dc) const
{
    CView::Dump(dc);
}

CVMguiDoc* CVMguiView::GetDocument() // non-debug version is inline
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CVMguiDoc)));
    return (CVMguiDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CVMguiView message handlers

//
// keyboard
//
void CVMguiView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if (pTheApp->m_pThreadMgr && pTheApp->m_bFocused)
    {
        ((CThreadMgr *)pTheApp->m_pThreadMgr)->m_pThreadDevice->PostThreadMessage(WM_USER_KEY, (WPARAM)nChar, (LPARAM)nFlags);
    }
    else
        CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CVMguiView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if (pTheApp->m_pThreadMgr && pTheApp->m_bFocused)
    {
        ((CThreadMgr *)pTheApp->m_pThreadMgr)->m_pThreadDevice->PostThreadMessage(WM_USER_KEY, (WPARAM)nChar, (LPARAM)nFlags);
    }
    else
        CView::OnKeyUp(nChar, nRepCnt, nFlags);
}

//
// mouse
//
void CVMguiView::OnMouseMove(UINT nFlags, CPoint point)
{
    if (pTheApp->m_pThreadMgr && pTheApp->m_bFocused)
    {
        RECT cRect;
        BOOL offScreen = FALSE;
        GetClientRect(&cRect);
        if (point.x < cRect.left)
        {
            offScreen = TRUE;
            point.x = cRect.left;
        }
        else if (point.x >= cRect.right)
        {
            offScreen = TRUE;
            point.x = cRect.right - 1;
        }
        if (point.y < cRect.top)
        {
            offScreen = TRUE;
            point.y = cRect.top;
        }
        else if (point.y >= cRect.bottom)
        {
            offScreen = TRUE;
            point.y = cRect.bottom - 1;
        }
        if (offScreen)
        {
            ClientToScreen(&point);
            SetCursorPos(point.x, point.y);
        }
        else
            ((CThreadMgr *)pTheApp->m_pThreadMgr)->m_pThreadDevice->PostThreadMessage(WM_USER_MOUSE_MOVE, (WPARAM)nFlags, (LPARAM)((point.x << 16) | (point.y & 0xFFFF)));
    }
    else
        CView::OnMouseMove(nFlags, point);
}


void CVMguiView::OnLButtonDown(UINT nFlags, CPoint point)
{
    if (pTheApp->m_pThreadMgr && pTheApp->m_bFocused)
    {
        ((CThreadMgr *)pTheApp->m_pThreadMgr)->m_pThreadDevice->PostThreadMessage(WM_USER_LEFT_BTN_DN, (WPARAM)nFlags, (LPARAM)((point.x << 16) | (point.y & 0xFFFF)));
    }
    else
        CView::OnLButtonDown(nFlags, point);
}

void CVMguiView::OnLButtonUp(UINT nFlags, CPoint point)
{
    if (pTheApp->m_pThreadMgr && pTheApp->m_bFocused)
    {
        ((CThreadMgr *)pTheApp->m_pThreadMgr)->m_pThreadDevice->PostThreadMessage(WM_USER_LEFT_BTN_UP, (WPARAM)nFlags, (LPARAM)((point.x << 16) | (point.y & 0xFFFF)));
    }
    else
        CView::OnLButtonUp(nFlags, point);
}

void CVMguiView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    if (pTheApp->m_pThreadMgr && pTheApp->m_bFocused)
    {
        ((CThreadMgr *)pTheApp->m_pThreadMgr)->m_pThreadDevice->PostThreadMessage(WM_USER_LEFT_BTN_DBL, (WPARAM)nFlags, (LPARAM)((point.x << 16) | (point.y & 0xFFFF)));
    }
    else
        CView::OnLButtonDblClk(nFlags, point);
}

void CVMguiView::OnRButtonDown(UINT nFlags, CPoint point)
{
    if (pTheApp->m_pThreadMgr && pTheApp->m_bFocused)
    {
        ((CThreadMgr *)pTheApp->m_pThreadMgr)->m_pThreadDevice->PostThreadMessage(WM_USER_RIGHT_BTN_DN, (WPARAM)nFlags, (LPARAM)((point.x << 16) | (point.y & 0xFFFF)));
    }
    else
        CView::OnRButtonDown(nFlags, point);
}

void CVMguiView::OnRButtonDblClk(UINT nFlags, CPoint point)
{
    if (pTheApp->m_pThreadMgr && pTheApp->m_bFocused)
    {
        ((CThreadMgr *)pTheApp->m_pThreadMgr)->m_pThreadDevice->PostThreadMessage(WM_USER_RIGHT_BTN_UP, (WPARAM)nFlags, (LPARAM)((point.x << 16) | (point.y & 0xFFFF)));
    }
    else
        CView::OnRButtonDblClk(nFlags, point);
}

void CVMguiView::OnRButtonUp(UINT nFlags, CPoint point)
{
    if (pTheApp->m_pThreadMgr && pTheApp->m_bFocused)
    {
        ((CThreadMgr *)pTheApp->m_pThreadMgr)->m_pThreadDevice->PostThreadMessage(WM_USER_RIGHT_BTN_DBL, (WPARAM)nFlags, (LPARAM)((point.x << 16) | (point.y & 0xFFFF)));
    }
    else
        CView::OnRButtonUp(nFlags, point);
}

//
// drawing
//

void CVMguiView::OnSize(UINT nType, int cx, int cy)
{
    CView::OnSize(nType, cx, cy);

    if (pTheApp->m_pThreadMgr)
        ((CThreadMgr *)pTheApp->m_pThreadMgr)->m_pThreadVGA->PostThreadMessage(WM_USER, USER_DRAW, 0);

}


void CVMguiView::OnEditCopy()
{
    if (OpenClipboard())
    {
        CListBox *pmyListBox = &pTheApp->m_pThreadMessages->m_pWndDbgMessages->m_dbg_messages;
        // Get the indexes of all the selected items.
        int nCount = pmyListBox->GetSelCount();

        int *aryListBoxSel = (int *)new int[nCount];

        pmyListBox->GetSelItems(nCount, aryListBoxSel);

        int totsize = 1;
        int i;
        for (i = 0; i < nCount; i++)
            totsize += pmyListBox->GetTextLen( aryListBoxSel[i] ) + 2;
        char *pbuf = new char[totsize];
        char *pb = pbuf;
        for (i = 0; i < nCount; i++)
        {
            pmyListBox->GetText( aryListBoxSel[i], pb );
            pb += pmyListBox->GetTextLen( aryListBoxSel[i] );
            *pb++ = '\n';
            *pb++ = '\r';
        }
        *pb = '\0';
        CString cs = pbuf;
        EmptyClipboard();
        // this is not working - an i am not sure why - does not work with CF_TEXT either
        SetClipboardData(CF_UNICODETEXT, (HANDLE)cs.operator LPCTSTR());
        CloseClipboard();
        delete aryListBoxSel;
        delete pbuf;
    }
}

