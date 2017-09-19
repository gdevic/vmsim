// VMgui.h : main header file for the VMGUI application
//

#if !defined(AFX_VMGUI_H__467C0D01_B147_41A4_A852_EAC2431A525B__INCLUDED_)
#define AFX_VMGUI_H__467C0D01_B147_41A4_A852_EAC2431A525B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
class CSimulator;
class CThreadMessages;


#define MAX_DISKS       6

typedef struct _DISKCONFIG
{
    DWORD   Flags;          // Disk structure flags:
    CString PathToFile;     // Path to the disk file
    int     hFile;          // Handle to a disk file
    FILE   *fpCache;        // File pointer to an optional write cache file
    DWORD  *pCacheIndex;    // Address of the optional cache index array
    DWORD   tracks;         // Number of tracks (aka. cylinders)
    DWORD   heads;          // Number of heads
    DWORD   sectors;        // Number of sectors per track
    DWORD   max_sectors;    // Total number of sectors on the disk

} DISKCONFIG, *PDISKCONFIG;

// Flags of a disk structure:

#define DSK_ENABLED         0x0001      // Disk is enabled
#define DSK_FLOPPY          0x0002      // This particular record represents a floppy
#define DSK_VIRTUAL         0x0004      // Disk is a virtual file
#define DSK_WRITECACHE      0x0008      // Undoable disks: cache all writes


typedef struct _VMCONFIG
{
    DWORD   PhysMem;        // Amount of VM physical memory in Mb (from config)
    CString PathToCMOS;     // Path to the CMOS data
    BYTE    CMOS[128];      // Default CMOS data

    DISKCONFIG Disk[MAX_DISKS]; // Virtual disks: 0,1: floppy, 2,3,4,5: hard disks

} VMCONFIG, *PVMCONFIG;

/////////////////////////////////////////////////////////////////////////////
// CVMguiApp:
// See VMgui.cpp for the implementation of this class
//

class CVMguiApp : public CWinApp
{
public:
    CVMguiApp();

    CWinThread              *m_pThreadMgr;
    CString                  m_sDriverName;
    BOOL                     m_bFocused;
    CThreadMessages         *m_pThreadMessages;

    CSimulator              *m_pSimulator;
    BOOL                     m_bExiting;
    CToolBarCtrl            *m_pwndToolBarCtrl;

    VMCONFIG                 m_VMConfig;
    CDocument               *m_pDoc;

public:
    void VmConfigCalcSectors();
    DWORD m_HostPhysMem;        // Amount of host physical memory
    DWORD m_HostFreePhysMem;    // Amount of free physical memory on the host
    void VmConfigClear();

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CVMguiApp)
    public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();
    //}}AFX_VIRTUAL

// Implementation
    COleTemplateServer m_server;
        // Server object for document creation
    //{{AFX_MSG(CVMguiApp)
    afx_msg void OnAppAbout();
    afx_msg void OnCtlLoad();
    afx_msg void OnCtlUnload();
    afx_msg void OnCtlPowerOff();
    afx_msg void OnCtlReset();
    afx_msg void OnCtlResume();
    afx_msg void OnCTLSENDCADel();
    afx_msg void OnViewDbgmssgs();
    afx_msg void OnViewMonochrome();
    afx_msg void OnCtlPowerOn();
    afx_msg void OnCtlSuspend();
    afx_msg void OnConfigMemory();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

// note: this can be used, or each class can use ((CVMguiApp *)AfxGetApp())
extern CVMguiApp    *pTheApp;

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VMGUI_H__467C0D01_B147_41A4_A852_EAC2431A525B__INCLUDED_)
