#if !defined(AFX_SIMULATOR_H__265D407C_5C57_46ED_96BE_D10E977CDD10__INCLUDED_)
#define AFX_SIMULATOR_H__265D407C_5C57_46ED_96BE_D10E977CDD10__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Simulator.h : header file
//
#include "vm.h"

/////////////////////////////////////////////////////////////////////////////
// CSimulator window

VOID CALLBACK RtcTimerA( HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime );

class CSimulator : public CWnd
{
// Construction
public:
    CSimulator();

// Attributes
public:
    HANDLE  sys_handle;
    TVM     *pVM;

// Operations
public:
    BOOL Load(CString driverName);
    BOOL Unload();
    BOOL Init();
    // runVM and SimMonitor run as separate threads
    UINT RunVM();
    UINT SimMonitor();
    // HandleRequest can be either synchronous or a separate thread
    void HandleRequest(PVMREQUEST pVmReq);
    // these are called and return immediately
    void StopVM();
    void ResetVM();
    void SuspendVM();
    void ResumeVM();

    void DiskOpenAll();
    void DiskCloseAll();
    void DiskRead( VMREQUEST *pVmReq );
    void DiskWrite( VMREQUEST *pVmReq );

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CSimulator)
    //}}AFX_VIRTUAL

// Implementation
public:
    void UpdateClock();
    HANDLE FloppyLock();
    CTime Clock;
    int RegisterMemoryCallback( DWORD physical_address, DWORD last_address,
                      TMEM_READ_Callback fnREAD, TMEM_WRITE_Callback fnWRITE );
    int RegisterIOPort(DWORD first_port, DWORD last_port,
                          TIO_IN_Callback fnIN, TIO_OUT_Callback fnOUT );
    int RegisterPCIDevice(DWORD device_number, TPCI_IN_Callback fnIN, TPCI_OUT_Callback fnOUT );
    int VmSIMGenerateInterrupt(DWORD IRQ_number);

    virtual ~CSimulator();

    // Generated message map functions
#define MIN_SECTOR_BUFFER   16384
protected:
    void DiskDisableError(PDISKCONFIG pDisk, const char *pMessage);
    BOOL DiskReadSectors(PDISKCONFIG pDisk, int offset, BYTE *pBuffer, int num_sectors = 1);
    void DiskOpen(PDISKCONFIG pDisk);

    DWORD  m_SectorBufferLen;
    BYTE * m_pSectorBuffer;
    //{{AFX_MSG(CSimulator)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// entry points for beginning threads
UINT RunVM(LPVOID lpvParam);
UINT SimMonitor(LPVOID lpvParam);


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SIMULATOR_H__265D407C_5C57_46ED_96BE_D10E977CDD10__INCLUDED_)
