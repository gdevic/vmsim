// ThreadVGA.cpp : implementation file
//

#include "stdafx.h"
#include "VMgui.h"
#include "UserCodes.h"
#include "ThreadVGA.h"
#include "mmsystem.h"
#include "ioctl.h"
#include "simulator.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CThreadVGA

IMPLEMENT_DYNCREATE(CThreadVGA, CWinThread)

CThreadVGA::CThreadVGA()
{
    m_timerIdVGAdraw = 0;
}

CThreadVGA::~CThreadVGA()
{
}

BOOL CThreadVGA::InitInstance()
{
    m_bAutoDelete = TRUE;
    if (!(m_pClientWnd = m_pMainWnd->GetWindow(GW_CHILD)))
        m_pClientWnd = m_pMainWnd;
    m_pClientWnd->Invalidate(TRUE);
    m_timerIdVGAdraw = timeSetEvent(VGA_TIME_TO_REPEAT, VGA_TIME_TO_REPEAT, TimerForVGAdraw, (DWORD)this, TIME_PERIODIC);
    return TRUE;
}

int CThreadVGA::ExitInstance()
{
    // TODO:  perform any per-thread cleanup here
    return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(CThreadVGA, CWinThread)
    //{{AFX_MSG_MAP(CThreadVGA)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
    ON_MESSAGE(WM_USER, OnUserMessage)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CThreadVGA message handlers

LRESULT CThreadVGA::OnUserMessage(WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
    case USER_EXIT:         // terminate
        if (m_timerIdVGAdraw)
        {
            timeKillEvent(m_timerIdVGAdraw);
            m_timerIdVGAdraw = 0;
        }
        if (!pTheApp->m_bExiting)
        {
            m_pClientWnd->Invalidate(TRUE);
        }
        AfxEndThread(0, TRUE);
        break;
    case USER_DRAW:     // draw client area
        DrawClientArea();
        break;
    case USER_SUSPEND:              // stop using timer updates
        if (m_timerIdVGAdraw)
        {
            timeKillEvent(m_timerIdVGAdraw);
            m_timerIdVGAdraw = 0;
        }
        m_pClientWnd->Invalidate(TRUE);
        break;
    case USER_RESUME:               // draw resume using timer area
        if (!m_timerIdVGAdraw)
        {
            m_timerIdVGAdraw = timeSetEvent(VGA_TIME_TO_REPEAT, VGA_TIME_TO_REPEAT, TimerForVGAdraw, (DWORD)this, TIME_PERIODIC);
        }
        break;
    default:
        break;
    }
    return NOERROR;
}



void CThreadVGA::DrawClientArea()
{
    if (pTheApp->m_bExiting)
        return;
    RECT cRect;
    RECT vgaRect = {0, 0, 640, 480};
    CBitmap destbmp;
    CBitmap srcbmp;
    CBitmap *oldbmp;
    CDC* pDC;
    CDC myDC;
    BITMAP bmp;
    CFont font;
    BOOL usingVGAtext = TRUE;

    m_pClientWnd->GetClientRect(&cRect);
    pDC = m_pClientWnd->GetDC();
    destbmp.CreateCompatibleBitmap(pDC, cRect.right, cRect.bottom);
    oldbmp = pDC->SelectObject(&destbmp);

    myDC.CreateCompatibleDC(pDC);
    srcbmp.CreateCompatibleBitmap(&myDC, vgaRect.right, vgaRect.bottom);
    srcbmp.GetBitmap(&bmp);
    myDC.SelectObject(&srcbmp);


    if (usingVGAtext)
    {
        font.CreatePointFont(10, _T("Courier"), &myDC);
        CFont *oldfont = myDC.SelectObject(&font);
        myDC.SetTextColor(RGB(0xff, 0xff, 0xff));           // white text
        myDC.FillSolidRect(&vgaRect, RGB(0x00,0x00,0x00));  // black back

        PBYTE pVGA = (PBYTE)&pTheApp->m_pSimulator->pVM->VgaTextBuffer;
        int step = 480/25;
        RECT tRect = {0, 0, 640, step};
        for (int r = 0; r < 25; r++)
        {
            char str[100];
            char *pch = str;
            for (int c = 0; c < 80; c++)
            {
                BYTE attr;
                *pch++ = *pVGA++;
                attr = *pVGA++;
            }
            *pch = '\0';
            myDC.DrawText(str, 80, &tRect, DT_LEFT);
            OffsetRect(&tRect, 0, step);
        }

        // must get rid of font object
        myDC.SelectObject(oldfont);
        font.DeleteObject();
    }
    else    // bitmap
    {
        int x,y;
        //      PBYTE pVGA = (PBYTE)&pTheApp->m_pSimulator->pVM->pMonitor->VgaBitBuffer;
        for (y = 0; y < vgaRect.bottom; y++)
        {
            for (x = 0; x < vgaRect.right; x++)
            {
                int r,g,b;
                r = x;//*pVGA       // get these from vga buffer mapped to 0 - 255
                g = y;
                b = (x*y) & 0xff;
                POINT p = {x,y};
#if 1//VERY_SLOW
                    myDC.SetPixel(p, RGB(r,g,b));
#endif
            }
        }
    }


    int nX = 0;
    int nY = 0;
    if (vgaRect.right < cRect.right)
    {
        nX = ((cRect.right - cRect.left) - vgaRect.right) / 2;
        cRect.right = vgaRect.right;
    }
    if (vgaRect.bottom < cRect.bottom)
    {
        nY = ((cRect.bottom - cRect.top) - vgaRect.bottom) / 2;
        cRect.bottom = vgaRect.bottom;
    }
    pDC->BitBlt(nX, nY, cRect.right, cRect.bottom, &myDC, 0, 0, SRCCOPY);
    pDC->SelectObject(oldbmp);
    m_pClientWnd->ReleaseDC(pDC);
}


void CALLBACK TimerForVGAdraw(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
    ((CThreadVGA *)dwUser)->DrawClientArea();
}


