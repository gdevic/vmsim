// VMgui.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "VMgui.h"

#include "UserCodes.h"
#include "ioctl.h"

#include "simulator.h"
#include "ThreadMgr.h"
#include "ThreadDevice.h"
#include "ThreadMessages.h"
#include "ThreadMonochrome.h"
#include "ThreadVGA.h"

#include "SettingsMachine.h"

#include "MainFrm.h"
#include "VMguiDoc.h"
#include "VMguiView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVMguiApp

BEGIN_MESSAGE_MAP(CVMguiApp, CWinApp)
    //{{AFX_MSG_MAP(CVMguiApp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND(ID_CTL_LOAD, OnCtlLoad)
    ON_COMMAND(ID_CTL_UNLOAD, OnCtlUnload)
    ON_COMMAND(ID_CTL_POWER_OFF, OnCtlPowerOff)
    ON_COMMAND(ID_CTL_RESET, OnCtlReset)
    ON_COMMAND(ID_CTL_RESUME, OnCtlResume)
    ON_COMMAND(ID_CTL_SEND_C_A_Del, OnCTLSENDCADel)
    ON_COMMAND(ID_VIEW_DBGMSSGS, OnViewDbgmssgs)
    ON_COMMAND(ID_VIEW_MONOCHROME, OnViewMonochrome)
    ON_COMMAND(ID_CTL_POWER_ON, OnCtlPowerOn)
    ON_COMMAND(ID_CTL_SUSPEND, OnCtlSuspend)
    //}}AFX_MSG_MAP
    // Standard file based document commands
    ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
    ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
    // Standard print setup command
    ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVMguiApp construction

CVMguiApp::CVMguiApp()
{
    // TODO: add construction code here,
    // Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CVMguiApp object

CVMguiApp theApp;
CVMguiApp   *pTheApp;

// This identifier was generated to be statistically unique for your app.
// You may change it if you prefer to choose a specific identifier.

// {77662953-4886-4234-B188-3FD5FB1F3946}
static const CLSID clsid =
{ 0x77662953, 0x4886, 0x4234, { 0xb1, 0x88, 0x3f, 0xd5, 0xfb, 0x1f, 0x39, 0x46 } };

/////////////////////////////////////////////////////////////////////////////
// CVMguiApp initialization

BOOL CVMguiApp::InitInstance()
{
    MEMORYSTATUS stat;
    m_bExiting = FALSE;
    m_pThreadMgr = NULL;
    m_pSimulator = NULL;
    pTheApp = &theApp;
    m_bFocused = FALSE;
    m_sDriverName = "E:\\3dfx\\devel\\vmsim\\VMgui\\VMCsim.sys";
    m_sDriverName = "C:\\VMCsim.sys";
    if (!AfxSocketInit())
    {
        AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
        return FALSE;
    }

    // Initialize OLE libraries
    if (!AfxOleInit())
    {
        AfxMessageBox(IDP_OLE_INIT_FAILED);
        return FALSE;
    }

    AfxEnableControlContainer();

    // Standard initialization
    // If you are not using these features and wish to reduce the size
    //  of your final executable, you should remove from the following
    //  the specific initialization routines you do not need.

#ifdef _AFXDLL
    Enable3dControls();         // Call this when using MFC in a shared DLL
#else
    Enable3dControlsStatic();   // Call this when linking to MFC statically
#endif

    // Change the registry key under which our settings are stored.
    // TODO: You should modify this string to be something appropriate
    // such as the name of your company or organization.
    SetRegistryKey(_T("Local AppWizard-Generated Applications"));

    LoadStdProfileSettings();  // Load standard INI file options (including MRU)

    // Register the application's document templates.  Document templates
    //  serve as the connection between documents, frame windows and views.

    CSingleDocTemplate* pDocTemplate;
    pDocTemplate = new CSingleDocTemplate(
        IDR_MAINFRAME,
        RUNTIME_CLASS(CVMguiDoc),
        RUNTIME_CLASS(CMainFrame),       // main SDI frame window
        RUNTIME_CLASS(CVMguiView));
    AddDocTemplate(pDocTemplate);

    // Connect the COleTemplateServer to the document template.
    //  The COleTemplateServer creates new documents on behalf
    //  of requesting OLE containers by using information
    //  specified in the document template.
    m_server.ConnectTemplate(clsid, pDocTemplate, TRUE);
    // Note: SDI applications register server objects only if /Embedding
    //   or /Automation is present on the command line.

    // Enable DDE Execute open
    EnableShellOpen();
    RegisterShellFileTypes(TRUE);

    // Parse command line for standard shell commands, DDE, file open
    CCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);

    // Check to see if launched as OLE server
    if (cmdInfo.m_bRunEmbedded || cmdInfo.m_bRunAutomated)
    {
        // Register all OLE server (factories) as running.  This enables the
        //  OLE libraries to create objects from other applications.
        COleTemplateServer::RegisterAll();

        // Application was run with /Embedding or /Automation.  Don't show the
        //  main window in this case.
        return TRUE;
    }

    // When a server application is launched stand-alone, it is a good idea
    //  to update the system registry in case it has been damaged.
    m_server.UpdateRegistry(OAT_DISPATCH_OBJECT);
    COleObjectFactory::UpdateRegistryAll();

    // Dispatch commands specified on the command line
    if (!ProcessShellCommand(cmdInfo))
        return FALSE;

    // The one and only window has been initialized, so show and update it.
    m_pMainWnd->ShowWindow(SW_SHOW);
    m_pMainWnd->UpdateWindow();

    // Enable drag/drop open
    m_pMainWnd->DragAcceptFiles();
    m_pThreadMessages   = (CThreadMessages      *)AfxBeginThread(RUNTIME_CLASS(CThreadMessages      ));

    m_pwndToolBarCtrl->SetState(ID_CTL_UNLOAD, TBSTATE_INDETERMINATE);
    m_pwndToolBarCtrl->SetState(ID_CTL_POWER_ON, TBSTATE_INDETERMINATE);
    m_pwndToolBarCtrl->SetState(ID_CTL_POWER_OFF, TBSTATE_INDETERMINATE);
    m_pwndToolBarCtrl->SetState(ID_CTL_FOCUS, TBSTATE_INDETERMINATE);
    m_pwndToolBarCtrl->SetState(ID_VIEW_MONOCHROME, TBSTATE_INDETERMINATE);
    m_pwndToolBarCtrl->SetState(ID_CTL_SUSPEND, TBSTATE_INDETERMINATE);
    m_pwndToolBarCtrl->SetState(ID_CTL_RESUME, TBSTATE_INDETERMINATE);

    m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_UNLOAD, MF_DISABLED | MF_GRAYED);
    m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_POWER_ON, MF_DISABLED | MF_GRAYED);
    m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_POWER_OFF, MF_DISABLED | MF_GRAYED);
    m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_FOCUS, MF_DISABLED | MF_GRAYED);
    m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_RESET, MF_DISABLED | MF_GRAYED);
    m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_SEND_C_A_Del, MF_DISABLED | MF_GRAYED);
    m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_SUSPEND, MF_DISABLED | MF_GRAYED);
    m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_RESUME, MF_DISABLED | MF_GRAYED);
    m_pMainWnd->GetMenu()->EnableMenuItem(ID_VIEW_MONOCHROME, MF_DISABLED | MF_GRAYED);
    m_pMainWnd->DrawMenuBar();

    // Get the amount of physical memory installed on this machine (in Mb)

    GlobalMemoryStatus (&stat);

    pTheApp->m_HostPhysMem = (stat.dwTotalPhys / (1024 * 1024)) + 1;
    pTheApp->m_HostFreePhysMem = (stat.dwAvailPhys / (1024 * 1024)) + 1;

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
    public:
    CAboutDlg();

    // Dialog Data
    //{{AFX_DATA(CAboutDlg)
    enum { IDD = IDD_ABOUTBOX
    };
    //}}AFX_DATA

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAboutDlg)
    protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

    // Implementation
    protected:
    //{{AFX_MSG(CAboutDlg)
    // No message handlers
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
    //{{AFX_DATA_INIT(CAboutDlg)
    //}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAboutDlg)
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
    //{{AFX_MSG_MAP(CAboutDlg)
        // No message handlers
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CVMguiApp::OnAppAbout()
{
    CAboutDlg aboutDlg;
    aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CVMguiApp message handlers

int CVMguiApp::ExitInstance()
{
    m_bExiting = TRUE;
    OnCtlPowerOff();
    OnCtlUnload();
    m_pThreadMessages->PostThreadMessage(WM_USER, USER_EXIT, 0);
    Sleep(200);
    if (!CWinApp::SaveAllModified())
    {
        m_bExiting = FALSE;
        return 1;
    }

    return CWinApp::ExitInstance();
}

// load and init the simulator
void CVMguiApp::OnCtlLoad()
{
    BOOL success = FALSE;
    if (m_pSimulator)
        return;
    m_pSimulator = new CSimulator;
    success = m_pSimulator->Load(m_sDriverName);
    if (success)
    {
        if (!(success = m_pSimulator->Init()))
            m_pSimulator->Unload();
    }
    if (!success)
    {
        delete m_pSimulator;
        m_pSimulator = NULL;
    }
    else
    {
        UINT val;
        val = m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_LOAD, MF_DISABLED | MF_GRAYED | MF_BYCOMMAND);
        val = m_pMainWnd->GetMenu()->CheckMenuItem(ID_CTL_LOAD, MF_CHECKED | MF_BYCOMMAND);
        m_pwndToolBarCtrl->SetState(ID_CTL_LOAD, TBSTATE_INDETERMINATE);
        m_pwndToolBarCtrl->SetState(ID_CTL_UNLOAD, TBSTATE_ENABLED);
        m_pwndToolBarCtrl->SetState(ID_CTL_POWER_ON, TBSTATE_ENABLED);

        m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_LOAD, MF_DISABLED | MF_GRAYED);
        m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_UNLOAD, MF_DISABLED | MF_GRAYED);
        m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_POWER_ON, MF_ENABLED);

        m_pMainWnd->GetMenu()->EnableMenuItem(ID_FILE_OPEN, MF_DISABLED | MF_GRAYED);
        m_pMainWnd->DrawMenuBar();
    }
}

// unload the simulator
void CVMguiApp::OnCtlUnload()
{
    if (m_pSimulator)
    {
        BOOL success = m_pSimulator->Unload();
        delete m_pSimulator;
        m_pSimulator = NULL;
    }
    if (!m_bExiting)
    {
        m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_LOAD, MF_ENABLED);
        UINT val;
        val = m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_POWER_ON, MF_DISABLED | MF_GRAYED | MF_BYCOMMAND);
        m_pwndToolBarCtrl->SetState(ID_CTL_LOAD, TBSTATE_ENABLED);
        m_pwndToolBarCtrl->SetState(ID_CTL_UNLOAD, TBSTATE_INDETERMINATE);
        m_pwndToolBarCtrl->SetState(ID_CTL_POWER_ON, TBSTATE_INDETERMINATE);

        m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_LOAD, MF_ENABLED);
        m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_UNLOAD, MF_DISABLED | MF_GRAYED);
        m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_POWER_ON, MF_DISABLED | MF_GRAYED);

        m_pMainWnd->GetMenu()->EnableMenuItem(ID_FILE_OPEN, MF_ENABLED);
        m_pMainWnd->DrawMenuBar();
    }

}

void CVMguiApp::OnCtlPowerOn()
{
    if (!m_pSimulator || m_pThreadMgr)
        return;
    m_pThreadMgr = AfxBeginThread(RUNTIME_CLASS(CThreadMgr));
    //, int nPriority = THREAD_PRIORITY_NORMAL, UINT nStackSize = 0, DWORD dwCreateFlags = 0, LPSECURITY_ATTRIBUTES lpSecurityAttrs = NULL );
    // give time for all the threads to start
    m_pwndToolBarCtrl->SetState(ID_CTL_UNLOAD, TBSTATE_INDETERMINATE);
    m_pwndToolBarCtrl->SetState(ID_CTL_POWER_ON, TBSTATE_INDETERMINATE);
    m_pwndToolBarCtrl->SetState(ID_CTL_POWER_OFF, TBSTATE_ENABLED);
    m_pwndToolBarCtrl->SetState(ID_CTL_FOCUS, TBSTATE_ENABLED);
    m_pwndToolBarCtrl->SetState(ID_VIEW_MONOCHROME, TBSTATE_ENABLED);
    m_pwndToolBarCtrl->SetState(ID_CTL_SUSPEND, TBSTATE_ENABLED);
    m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_UNLOAD, MF_DISABLED | MF_GRAYED);
    m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_POWER_ON, MF_DISABLED | MF_GRAYED);
    m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_POWER_OFF, MF_ENABLED);
    m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_FOCUS, MF_ENABLED);
    m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_RESET, MF_ENABLED);
    m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_SEND_C_A_Del, MF_ENABLED);
    m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_SUSPEND, MF_ENABLED);
    m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_RESUME, MF_DISABLED | MF_GRAYED);
    m_pMainWnd->GetMenu()->EnableMenuItem(ID_VIEW_MONOCHROME, MF_ENABLED);
    Sleep(200);
}

void CVMguiApp::OnCtlPowerOff()
{
    ReleaseCapture();
    if (m_pThreadMgr)
    {
        m_pThreadMgr->PostThreadMessage(WM_USER, USER_EXIT, 0);
        m_pThreadMgr = NULL;
        // give time for all the threads to stop
        Sleep(200);
    }
    if (!m_bExiting)
    {
        m_pwndToolBarCtrl->SetState(ID_CTL_UNLOAD, TBSTATE_ENABLED);
        m_pwndToolBarCtrl->SetState(ID_CTL_POWER_ON, TBSTATE_ENABLED);
        m_pwndToolBarCtrl->SetState(ID_CTL_POWER_OFF, TBSTATE_INDETERMINATE);
        m_pwndToolBarCtrl->SetState(ID_CTL_FOCUS, TBSTATE_INDETERMINATE);
        m_pwndToolBarCtrl->SetState(ID_VIEW_MONOCHROME, TBSTATE_INDETERMINATE);
        m_pwndToolBarCtrl->SetState(ID_CTL_SUSPEND, TBSTATE_INDETERMINATE);
        m_pwndToolBarCtrl->SetState(ID_CTL_RESUME, TBSTATE_INDETERMINATE);
        m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_UNLOAD, MF_ENABLED);
        m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_POWER_ON, MF_ENABLED);
        m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_POWER_OFF, MF_DISABLED | MF_GRAYED);
        m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_FOCUS, MF_DISABLED | MF_GRAYED);
        m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_RESET, MF_DISABLED | MF_GRAYED);
        m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_SEND_C_A_Del, MF_DISABLED | MF_GRAYED);
        m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_SUSPEND, MF_DISABLED | MF_GRAYED);
        m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_RESUME, MF_DISABLED | MF_GRAYED);
        m_pMainWnd->GetMenu()->EnableMenuItem(ID_VIEW_MONOCHROME, MF_DISABLED | MF_GRAYED);
    }

}

void CVMguiApp::OnCtlReset()
{
    if (m_pSimulator)
        m_pSimulator->ResetVM();

}


void CVMguiApp::OnCtlSuspend()
{
    if (m_pSimulator)
    {
        m_pSimulator->SuspendVM();
        m_pwndToolBarCtrl->SetState(ID_CTL_FOCUS, TBSTATE_INDETERMINATE);
        m_pwndToolBarCtrl->SetState(ID_CTL_SUSPEND, TBSTATE_INDETERMINATE);
        m_pwndToolBarCtrl->SetState(ID_CTL_RESUME, TBSTATE_ENABLED);
        m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_FOCUS, MF_DISABLED | MF_GRAYED);
        m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_RESET, MF_DISABLED | MF_GRAYED);
        m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_SEND_C_A_Del, MF_DISABLED | MF_GRAYED);
        m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_SUSPEND, MF_DISABLED | MF_GRAYED);
        m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_RESUME, MF_ENABLED);
    }
    if (m_pThreadMgr)
    {
        ((CThreadMgr *)m_pThreadMgr)->m_pThreadVGA->PostThreadMessage(WM_USER, USER_SUSPEND, 0);
        ((CThreadMgr *)m_pThreadMgr)->m_pThreadMonochrome->PostThreadMessage(WM_USER, USER_SUSPEND, 0);
    }

}

void CVMguiApp::OnCtlResume()
{
    if (m_pSimulator)
    {
        m_pSimulator->ResumeVM();
        m_pwndToolBarCtrl->SetState(ID_CTL_FOCUS, TBSTATE_ENABLED);
        m_pwndToolBarCtrl->SetState(ID_CTL_SUSPEND, TBSTATE_ENABLED);
        m_pwndToolBarCtrl->SetState(ID_CTL_RESUME, TBSTATE_INDETERMINATE);
        m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_FOCUS, MF_ENABLED);
        m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_RESET, MF_ENABLED);
        m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_SEND_C_A_Del, MF_ENABLED);
        m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_SUSPEND, MF_ENABLED);
        m_pMainWnd->GetMenu()->EnableMenuItem(ID_CTL_RESUME, MF_DISABLED | MF_GRAYED);
    }
    if (m_pThreadMgr)
    {
        ((CThreadMgr *)m_pThreadMgr)->m_pThreadMonochrome->PostThreadMessage(WM_USER, USER_RESUME, 0);
        ((CThreadMgr *)m_pThreadMgr)->m_pThreadVGA->PostThreadMessage(WM_USER, USER_RESUME, 0);
    }

}

void CVMguiApp::OnCTLSENDCADel()
{
    if (m_pThreadMgr)
    {
        ((CThreadMgr *)m_pThreadMgr)->m_pThreadDevice->PostThreadMessage(WM_USER, USER_CTRL_ALT_DEL, 0);
    }

}


void CVMguiApp::OnViewDbgmssgs()
{
    m_pThreadMessages->PostThreadMessage(WM_USER, USER_SHOW_WINDOW, 0);
}

void CVMguiApp::OnViewMonochrome()
{
    if (m_pThreadMgr)
    {
        ((CThreadMgr *)m_pThreadMgr)->m_pThreadMonochrome->PostThreadMessage(WM_USER, USER_SHOW_WINDOW, 0);
    }

}



void CVMguiApp::OnConfigMemory()
{
    CSettingsMachine dlg;
    if (dlg.DoModal() == IDOK)
        m_pDoc->SetModifiedFlag(TRUE);

}

// Initializes VmConfig data structure clean

void CVMguiApp::VmConfigClear()
{
    PVMCONFIG p = &m_VMConfig;
    PDISKCONFIG d;

    p->PhysMem = 0;
    p->PathToCMOS.Empty();
    memset(p->CMOS, 0, sizeof(p->CMOS));

    for(int n=0; n<MAX_DISKS; n++)
    {
        d = &p->Disk[n];

        d->Flags = 0;
        d->PathToFile.Empty();
        d->hFile = 0;
        d->fpCache = NULL;
        d->pCacheIndex = NULL;
        d->tracks = 0;
        d->heads = 0;
        d->sectors = 0;
    }

    p->Disk[0].Flags |= DSK_FLOPPY;
    p->Disk[0].tracks  = MAX_FLOPPY_TRACKS;
    p->Disk[0].heads   = MAX_FLOPPY_HEADS;
    p->Disk[0].sectors = MAX_FLOPPY_SECTORS;
    p->Disk[0].max_sectors = MAX_FLOPPY_TRACKS * MAX_FLOPPY_HEADS * MAX_FLOPPY_SECTORS;

    p->Disk[1].Flags |= DSK_FLOPPY;
    p->Disk[1].tracks  = MAX_FLOPPY_TRACKS;
    p->Disk[1].heads   = MAX_FLOPPY_HEADS;
    p->Disk[1].sectors = MAX_FLOPPY_SECTORS;
    p->Disk[1].max_sectors = MAX_FLOPPY_TRACKS * MAX_FLOPPY_HEADS * MAX_FLOPPY_SECTORS;
}

void CVMguiApp::VmConfigCalcSectors()
{
    PVMCONFIG p = &m_VMConfig;
    PDISKCONFIG d;

    for(int n=0; n<MAX_DISKS; n++)
    {
        d = &p->Disk[n];

        d->max_sectors = d->tracks * d->heads * d->sectors;
    }
}

