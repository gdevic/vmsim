// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "VMgui.h"
#include "UserCodes.h"
#include "ThreadDevice.h"
#include "ThreadMgr.h"
#include "MainFrm.h"

#include "Progress.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    //{{AFX_MSG_MAP(CMainFrame)
    ON_WM_CREATE()
    ON_COMMAND(ID_CTL_FOCUS, OnCtlFocus)
    ON_COMMAND(ID_KEY_END_FOCUS, OnKeyEndFocus)
    ON_COMMAND(ID_SETTINGS_VM, OnSettingsVm)
    ON_COMMAND(ID_SETTINGS_SYSTEM, OnSettingsSystem)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

static UINT indicators[] =
{
    ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_FLOPPY,
	ID_INDICATOR_HDD,
    ID_INDICATOR_CAPS,
    ID_INDICATOR_NUM,
    ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
    // TODO: add member initialization code here

}

CMainFrame::~CMainFrame()
{

}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
        | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
        !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
    {
        TRACE0("Failed to create toolbar\n");
        return -1;      // fail to create
    }

    if (!m_wndStatusBar.Create(this) ||
        !m_wndStatusBar.SetIndicators(indicators,
        sizeof(indicators)/sizeof(UINT)))
    {
        TRACE0("Failed to create status bar\n");
        return -1;      // fail to create
    }

    // TODO: Delete these three lines if you don't want the toolbar to
    //  be dockable
    m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
    EnableDocking(CBRS_ALIGN_ANY);
    DockControlBar(&m_wndToolBar);

    pTheApp->m_pwndToolBarCtrl = &m_wndToolBar.GetToolBarCtrl();
    m_bAutoMenuEnable = FALSE;
    return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    if( !CFrameWnd::PreCreateWindow(cs) )
        return FALSE;
    // TODO: Modify the Window class or styles here by modifying
    //  the CREATESTRUCT cs
    CString csloc;
    csloc.Format(_T("LocFor %dx%d"), GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));

    int x = AfxGetApp()->GetProfileInt(csloc, _T("Main X"), 0);
    int y = AfxGetApp()->GetProfileInt(csloc, _T("Main Y"), 0);
    int w = AfxGetApp()->GetProfileInt(csloc, _T("Main W"), 0);
    int h = AfxGetApp()->GetProfileInt(csloc, _T("Main H"), 0);
    if (w && h)
    {
        cs.x = x;
        cs.y = y;
        cs.cx = w;
        cs.cy = h;
    }
    cs.lpszName = "VMsim";

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
    CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
    CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers


BOOL CMainFrame::DestroyWindow()
{
    RECT MainR;
    GetWindowRect(&MainR);
    CString csloc;
    csloc.Format(_T("LocFor %dx%d"), GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    AfxGetApp()->WriteProfileInt(csloc, _T("Main X"), MainR.left);
    AfxGetApp()->WriteProfileInt(csloc, _T("Main Y"), MainR.top);
    AfxGetApp()->WriteProfileInt(csloc, _T("Main W"), MainR.right - MainR.left);
    AfxGetApp()->WriteProfileInt(csloc, _T("Main H"), MainR.bottom - MainR.top);

    return CFrameWnd::DestroyWindow();
}

void CMainFrame::OnCtlFocus()
{
    GetActiveView()->SetCapture();
    pTheApp->m_bFocused = TRUE;
    m_wndStatusBar.SetPaneText(0, _T("Press Ctrl-Alt-Esc to release input"), TRUE);

}

void CMainFrame::OnKeyEndFocus()
{
    ReleaseCapture();
    pTheApp->m_bFocused = FALSE;
    m_wndStatusBar.SetPaneText(0, _T(""), TRUE);

    // Send a message to stuff Control+Alt break codes into the VM

    ((CThreadMgr *)pTheApp->m_pThreadMgr)->m_pThreadDevice->PostThreadMessage(WM_USER_KEY, (WPARAM)0, (LPARAM)0);
}


void CMainFrame::OnSettingsVm()
{
    // TODO: Add your command handler code here

    CSettingsVM SettingsVM(_T("Virtual Machine Settings"));

    if( SettingsVM.DoModal() == IDOK )
    {
        // Accept new values for any changed data
        PVMCONFIG p = &pTheApp->m_VMConfig;

        //====================== MEMORY CONFIG ===============================
        CTabVmm *pVmm = &SettingsVM.m_tabVmm;

        p->PathToCMOS = pVmm->m_FileNameCMOS;
        p->PhysMem    = pVmm->m_PhysMem;

        //====================== FLOPPY DISKS ================================
        CTabFloppy *pF;

        pF = SettingsVM.m_tabFloppy[0];
        p->Disk[0].Flags = (pF->m_Enabled ? DSK_ENABLED : 0)
            |              (pF->m_Virtual ? DSK_VIRTUAL : 0)
            |              (pF->m_Undoable? DSK_WRITECACHE : 0)
            |              DSK_FLOPPY;
        p->Disk[0].PathToFile = pF->m_FileName;

        pF = SettingsVM.m_tabFloppy[1];
        p->Disk[1].Flags = (pF->m_Enabled ? DSK_ENABLED : 0)
            |              (pF->m_Virtual ? DSK_VIRTUAL : 0)
            |              (pF->m_Undoable? DSK_WRITECACHE : 0)
            |              DSK_FLOPPY;
        p->Disk[1].PathToFile = pF->m_FileName;

        //====================== HDD DISKS ==================================
        CTabIde *pI;

        pI = SettingsVM.m_tabIde[0];
        p->Disk[2].Flags = (pI->m_Enabled ? DSK_ENABLED : 0)
            |              (pI->m_Virtual ? DSK_VIRTUAL : 0)
            |              (pI->m_Undoable? DSK_WRITECACHE : 0);
        p->Disk[2].tracks = pI->m_Tracks;
        p->Disk[2].heads = pI->m_Heads;
        p->Disk[2].sectors = pI->m_Sectors;
        p->Disk[2].PathToFile = pI->m_FileName;

        pI = SettingsVM.m_tabIde[1];
        p->Disk[3].Flags = (pI->m_Enabled ? DSK_ENABLED : 0)
            |              (pI->m_Virtual ? DSK_VIRTUAL : 0)
            |              (pI->m_Undoable? DSK_WRITECACHE : 0);
        p->Disk[3].tracks = pI->m_Tracks;
        p->Disk[3].heads = pI->m_Heads;
        p->Disk[3].sectors = pI->m_Sectors;
        p->Disk[3].PathToFile = pI->m_FileName;

        pI = SettingsVM.m_tabIde[2];
        p->Disk[4].Flags = (pI->m_Enabled ? DSK_ENABLED : 0)
            |              (pI->m_Virtual ? DSK_VIRTUAL : 0)
            |              (pI->m_Undoable? DSK_WRITECACHE : 0);
        p->Disk[4].tracks = pI->m_Tracks;
        p->Disk[4].heads = pI->m_Heads;
        p->Disk[4].sectors = pI->m_Sectors;
        p->Disk[4].PathToFile = pI->m_FileName;

        pI = SettingsVM.m_tabIde[3];
        p->Disk[5].Flags = (pI->m_Enabled ? DSK_ENABLED : 0)
            |              (pI->m_Virtual ? DSK_VIRTUAL : 0)
            |              (pI->m_Undoable? DSK_WRITECACHE : 0);
        p->Disk[5].tracks = pI->m_Tracks;
        p->Disk[5].heads = pI->m_Heads;
        p->Disk[5].sectors = pI->m_Sectors;
        p->Disk[5].PathToFile = pI->m_FileName;

        pTheApp->VmConfigCalcSectors();

        // Fixup the CMOS data depending on the new disk selection

        pTheApp->m_VMConfig.CMOS[0x10] = 0;
        if(p->Disk[0].Flags & DSK_ENABLED)
            pTheApp->m_VMConfig.CMOS[0x10] |= 0x04,
            pTheApp->m_VMConfig.CMOS[0x14] &= ~3;
        if(p->Disk[1].Flags & DSK_ENABLED)
            pTheApp->m_VMConfig.CMOS[0x10] |= 0x40,
            pTheApp->m_VMConfig.CMOS[0x14] |= 0x01;
    }
}

void CMainFrame::OnSettingsSystem()
{
    // TODO: Add your command handler code here

    CProgress * p = new CProgress();

    p->Create();

    CString s = _T("Hello");

    for(int i=0; i<100; i++)
    {
        s.Format("Hello %d", i);
        p->SetText((LPCTSTR)s);
        Sleep(20);
        p->SetPos(i);
    }

    delete p;
}

