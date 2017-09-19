// Simulator.cpp : implementation file
//

#include "stdafx.h"
#include "VMgui.h"


#include <winioctl.h>
#include "Ioctl.h"
#include "vm.h"
#include "InstDrv.h"
#include "Functions.h"                  // Include all the function decl
#include "UserCodes.h"

#include "Simulator.h"
#include "ThreadMessages.h"
#include "ThreadMgr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Variables/definitions for the device driver

#define        SYS_FILE     TEXT("VMCsim.sys")
#define        SYS_NAME     TEXT("VMCSIM")

////////////////////////////////////////////////////////////////////////////
// CSimulator

CSimulator::CSimulator()
{
    sys_handle = INVALID_HANDLE_VALUE;
    pVM = NULL;

    // Initially allocate some read buffer
    m_pSectorBuffer = (BYTE *) malloc(MIN_SECTOR_BUFFER);
    m_SectorBufferLen = MIN_SECTOR_BUFFER;

    ASSERT(m_pSectorBuffer);
}

CSimulator::~CSimulator()
{
    free(m_pSectorBuffer);

    Unload();
}


BEGIN_MESSAGE_MAP(CSimulator, CWnd)
    //{{AFX_MSG_MAP(CSimulator)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CSimulator message handlers

/////////////////////////////////////////////////////////////////////////
BOOL CSimulator::Load(CString driverName)
{
    // Load the device driver
    CString msgbuf;
    int error;
    if ( (error = LoadDeviceDriver( SYS_NAME, driverName, &sys_handle )) != 0)
    {
        CFileDialog dlg(TRUE, "*.sys", "*.sys", OFN_ENABLESIZING | OFN_FILEMUSTEXIST |
            OFN_NONETWORKBUTTON | OFN_PATHMUSTEXIST, "VM Simulator Core, Device Driver (*.sys)");

        UnloadDeviceDriver( SYS_NAME, sys_handle );

        if( dlg.DoModal() == IDOK )
        {
            if ( (error = LoadDeviceDriver( SYS_NAME, dlg.GetPathName(), &sys_handle )) != 0)
            {
                msgbuf.Format(TEXT("\nError loading %s (%s, %d)\nTry to load it again."), dlg.GetPathName(), SYS_NAME, error );
                if( error == ERROR_FILE_NOT_FOUND )
                    msgbuf = TEXT("Device driver file not found") + msgbuf;
                else
                    if( error == ERROR_SERVICE_DISABLED )
                    msgbuf = TEXT("Service disabled (Non-local drive?)") + msgbuf;

                AfxMessageBox(msgbuf);

                UnloadDeviceDriver( SYS_NAME, sys_handle );
                sys_handle = INVALID_HANDLE_VALUE;
                return(FALSE);
            }
            return TRUE;
        }
        sys_handle = INVALID_HANDLE_VALUE;
        return FALSE;
    }
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////
BOOL CSimulator::Unload()
{
    if (pVM)
    {
        Free_VM(pVM);
        pVM = NULL;
    }

    if (sys_handle != INVALID_HANDLE_VALUE)
    {
        UnloadDeviceDriver( SYS_NAME, sys_handle );
        sys_handle = INVALID_HANDLE_VALUE;
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////
BOOL CSimulator::Init()
{
    DWORD nb;
    OVERLAPPED overlap = { 0 };
    int error;

    // Initialize virtual machine device driver

    pVM = Init_VM();

    if( pVM != NULL )
    {
        // Check the memory allocation links

        DeviceIoControl( sys_handle, VMSIM_MemCheck,
            NULL, 0,
            &error, 4,          // check code is in `error'
            &nb, &overlap );    // `nb' ignored

        if( error == 0 )
        {
            // Initialize virtual machine subsystems:
            //-----------------------------------------------------------
            // Initialize clock embedded in the VM's CMOS

            UpdateClock();

            //-----------------------------------------------------------
            // Open all the file handles for virtual disks

            DiskOpenAll();

            return( TRUE );
        }
    }

    // We failed somewhere...

    return FALSE;
}


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
// this runs in a separate thread until an iocontrol terminates the run
UINT CSimulator::RunVM()
{
    DWORD nb;
    VMREQUEST vmReq;
    VMCONTROL vmCtrl;
    DWORD status;
    OVERLAPPED overlap = { 0 };

    overlap.hEvent = CreateEvent(NULL, TRUE, FALSE, _T("RunVMOverlapEvent"));

    // Send a message that we are about to start a VM
    vmCtrl.type = CtlStartVM;
    status = DeviceIoControl( sys_handle, VMSIM_Control,
        &vmCtrl, sizeof(VMCONTROL),
        NULL, 0,
        &nb, &overlap );
    status = GetOverlappedResult(sys_handle, &overlap, &nb, TRUE);      // wait
    if(!status)
        status = GetLastError();

    for (;;)
    {
        memset(&vmReq, 0, sizeof(VMREQUEST));

        status = DeviceIoControl( sys_handle, VMSIM_RunVM,
            pVM, sizeof(TVM),
            &vmReq, sizeof(VMREQUEST),
            &nb, &overlap );
        status = GetOverlappedResult(sys_handle, &overlap, &nb, TRUE);      // wait
        if(!status)
            status = GetLastError();

        if (vmReq.head.type == ReqVMExit)
            return 0;
        HandleRequest(&vmReq);
    }
    return 0;
}

/////////////////////////////////////////////////////////////////////////
// this runs in a separate thread until an iocontrol terminates the run
UINT CSimulator::SimMonitor()
{
    DWORD nb;
    VMREQUEST vmReq;
    DWORD status;
    OVERLAPPED overlap = { 0 };

    overlap.hEvent = CreateEvent(NULL, TRUE, FALSE, _T("SimMonitorOverlapEvent"));

    for (;;)
    {
        memset(&vmReq, 0, sizeof(VMREQUEST));

        status = DeviceIoControl( sys_handle, VMSIM_GetVMRequest,
            0, NULL,
            &vmReq, sizeof(VMREQUEST),
            &nb, &overlap );
        status = GetOverlappedResult(sys_handle, &overlap, &nb, TRUE);      // wait
        if(!status)
            status = GetLastError();

        if (vmReq.head.type == ReqVMExit)
            return 0;
        HandleRequest(&vmReq);
    }
    return 0;
}


/////////////////////////////////////////////////////////////////////////
// this can be run as a separate thread or synchronously
// it handles the request and completes
void CSimulator::HandleRequest(PVMREQUEST pVmReq)
{
    static DWORD requestID = 0;
    OVERLAPPED overlap = { 0 };
    char str[256];

    if (pTheApp->m_bExiting)
        return;

    switch (pVmReq->head.type)
    {
    case ReqVMSuspended:
        // Device driver detected the suspended state.  In order to keep the
        // RunVM() thread alive, we will simply sleep and poll the resume state
        Sleep(100);
        break;

    case ReqVMMessage:
        memcpy(str, pVmReq->buffer, pVmReq->head.bufferlen);
        str[pVmReq->head.bufferlen] = '\0';
        pTheApp->m_pThreadMessages->PostThreadMessage(WM_USER_DBG_MESSAGE,
            (WPARAM)((pVmReq->head.flags << 8) | (pVmReq->head.bufferlen & 0xFF)), (LPARAM)(&str));
        break;

    case ReqVMReadDisk:             // Read sector

        sprintf(str, "Read%d: %d [%d] -> %6X", pVmReq->head.drive, pVmReq->head.offset, pVmReq->head.offset/512, pVmReq->head.flags );
        pTheApp->m_pThreadMessages->PostThreadMessage(WM_USER_DBG_MESSAGE, (WPARAM)(0), (LPARAM)(&str));

        DiskRead(pVmReq);
        break;

    case ReqVMWriteDisk:            // Write sector

        sprintf(str, "Write%d: %d [%d]", pVmReq->head.drive, pVmReq->head.offset, pVmReq->head.offset/512 );
        pTheApp->m_pThreadMessages->PostThreadMessage(WM_USER_DBG_MESSAGE, (WPARAM)(0), (LPARAM)(&str));

        DiskWrite(pVmReq);
        break;

    default:
        ASSERT(0);
    }
}


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
// functions called to start threads
UINT RunVM(LPVOID lpvParam)
{
    CSimulator *pSimulator = (CSimulator *)lpvParam;
    return pSimulator->RunVM();
}


/////////////////////////////////////////////////////////////////////////
UINT SimMonitor(LPVOID lpvParam)
{
    CSimulator *pSimulator = (CSimulator *)lpvParam;
    return pSimulator->SimMonitor();
}



/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
// functions that are called for immediate execution

// this stops the VM - causing all DeviceIo to complete
void CSimulator::StopVM()
{
    DWORD nb, status;
    VMCONTROL vmCtrl;
    CString msgbuf;
    OVERLAPPED overlap = { 0 };

    vmCtrl.type = CtlStopVM;
    status = DeviceIoControl( sys_handle, VMSIM_Control,
        &vmCtrl, sizeof(VMCONTROL),
        NULL, 0,
        &nb, &overlap );
    status = GetOverlappedResult(sys_handle, &overlap, &nb, TRUE);      // wait
    if(!status)
        status = GetLastError();

    // Give a device driver some time to clean the house

    Sleep(100);

    //-------------------------------------------------------------------
    // Close virtual disk files

    DiskCloseAll();


    //-------------------------------------------------------------------
    // make sure device cleans up

    status = DeviceIoControl( sys_handle, VMSIM_CleanItUp,
        NULL, 0,
        NULL, 0,
        &nb, &overlap );
    status = GetOverlappedResult(sys_handle, &overlap, &nb, TRUE);      // wait
    if(!status)
        status = GetLastError();
}


void CSimulator::ResetVM()
{
    DWORD nb;
    VMCONTROL vmCtrl;
    DWORD status;
    OVERLAPPED overlap = { 0 };

    vmCtrl.type = CtlResetVM;
    vmCtrl.flags = 0;
    status = DeviceIoControl( sys_handle, VMSIM_Control,
        &vmCtrl, sizeof(VMCONTROL),
        NULL, 0,
        &nb, &overlap );
    status = GetOverlappedResult(sys_handle, &overlap, &nb, TRUE);      // wait
    if(!status)
        status = GetLastError();
}


void CSimulator::SuspendVM()
{
    DWORD nb;
    VMCONTROL vmCtrl;
    DWORD status;
    OVERLAPPED overlap = { 0 };

    vmCtrl.type = CtlSuspendVM;
    vmCtrl.flags = 0;
    status = DeviceIoControl( sys_handle, VMSIM_Control,
        &vmCtrl, sizeof(VMCONTROL),
        NULL, 0,
        &nb, &overlap );
    status = GetOverlappedResult(sys_handle, &overlap, &nb, TRUE);      // wait
    if(!status)
        status = GetLastError();
}

void CSimulator::ResumeVM()
{
    DWORD nb;
    VMCONTROL vmCtrl;
    DWORD status;
    OVERLAPPED overlap = { 0 };

    vmCtrl.type = CtlResumeVM;
    vmCtrl.flags = 0;
    status = DeviceIoControl( sys_handle, VMSIM_Control,
        &vmCtrl, sizeof(VMCONTROL),
        NULL, 0,
        &nb, &overlap );
    status = GetOverlappedResult(sys_handle, &overlap, &nb, TRUE);      // wait
    if(!status)
        status = GetLastError();
}


