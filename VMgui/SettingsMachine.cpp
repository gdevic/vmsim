// SettingsMachine.cpp : implementation file
//

#include "stdafx.h"
#include "VMgui.h"
#include "SettingsMachine.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSettingsMachine dialog


CSettingsMachine::CSettingsMachine(CWnd* pParent /*=NULL*/)
    : CDialog(CSettingsMachine::IDD, pParent)
{
    //{{AFX_DATA_INIT(CSettingsMachine)
    m_phys_mem = 0;
    m_HostFreePhysMem = _T("");
    m_HostPhysMem = _T("");
    //}}AFX_DATA_INIT

    // Display the assigned amount of VM physical memory

    m_phys_mem = pTheApp->m_VMConfig.PhysMem;

    // Display the amount of host physical memory

    m_HostPhysMem.Format("Host Total Physical Memory (Mb): %d",
        pTheApp->m_HostPhysMem);

    m_HostFreePhysMem.Format("Host Free Physical Memory (Mb): %d",
        pTheApp->m_HostFreePhysMem);
}


void CSettingsMachine::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CSettingsMachine)
    DDX_Text(pDX, IDC_PHYS_MEM, m_phys_mem);
    DDV_MinMaxUInt(pDX, m_phys_mem, 4, 256);
    DDX_Text(pDX, IDC_HOST_FREE_PHYS_MEM, m_HostFreePhysMem);
    DDV_MaxChars(pDX, m_HostFreePhysMem, 128);
    DDX_Text(pDX, IDC_HOST_PHYS_MEM, m_HostPhysMem);
    DDV_MaxChars(pDX, m_HostPhysMem, 128);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSettingsMachine, CDialog)
    //{{AFX_MSG_MAP(CSettingsMachine)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSettingsMachine message handlers

void CSettingsMachine::OnOK()
{
    UpdateData(TRUE);
    pTheApp->m_VMConfig.PhysMem = m_phys_mem;
    CDialog::OnOK();
}

