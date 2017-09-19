// VMguiDoc.cpp : implementation of the CVMguiDoc class
//

#include "stdafx.h"
#include "VMgui.h"

#include "VMguiDoc.h"
#include "Vm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVMguiDoc

IMPLEMENT_DYNCREATE(CVMguiDoc, CDocument)

BEGIN_MESSAGE_MAP(CVMguiDoc, CDocument)
    //{{AFX_MSG_MAP(CVMguiDoc)
        // NOTE - the ClassWizard will add and remove mapping macros here.
        //    DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CVMguiDoc, CDocument)
    //{{AFX_DISPATCH_MAP(CVMguiDoc)
        // NOTE - the ClassWizard will add and remove mapping macros here.
        //      DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IVMgui to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the
//  dispinterface in the .ODL file.

// {99ACA9F5-0952-4997-BFC5-11A4DA37A770}
static const IID IID_IVMgui =
{ 0x99aca9f5, 0x952, 0x4997, { 0xbf, 0xc5, 0x11, 0xa4, 0xda, 0x37, 0xa7, 0x70 } };

BEGIN_INTERFACE_MAP(CVMguiDoc, CDocument)
    INTERFACE_PART(CVMguiDoc, IID_IVMgui, Dispatch)
END_INTERFACE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVMguiDoc construction/destruction

CVMguiDoc::CVMguiDoc()
{
    // TODO: add one-time construction code here
    pTheApp->m_pDoc = this;

    EnableAutomation();

    AfxOleLockApp();
}

CVMguiDoc::~CVMguiDoc()
{
    AfxOleUnlockApp();
}


/////////////////////////////////////////////////////////////////////////////
// CVMguiDoc serialization

void CVMguiDoc::Serialize(CArchive& ar)
{
    if (ar.IsStoring())
    {
        // TODO: add storing code here
    }
    else
    {
        // TODO: add loading code here
    }
}

/////////////////////////////////////////////////////////////////////////////
// CVMguiDoc diagnostics

#ifdef _DEBUG
void CVMguiDoc::AssertValid() const
{
    CDocument::AssertValid();
}

void CVMguiDoc::Dump(CDumpContext& dc) const
{
    CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CVMguiDoc commands

BOOL CVMguiDoc::OnNewDocument()
{
    HMODULE hInstance;
    HRSRC hFound;
    HGLOBAL hRes;
    DWORD dwResourceSize;
    LPVOID lpBuff;

    if (!CDocument::OnNewDocument())
        return FALSE;

    // Initialize default configuration:

    pTheApp->VmConfigClear();

    // We keep an image of CMOS state and an image of a 1.44 floppy disk in
    // our resources...
    PVMCONFIG p = &pTheApp->m_VMConfig;

    p->PhysMem = 4;
    //    p->PathToCMOS = _tempnam("C:\\", "VmCmos");
    p->PathToCMOS = "C:\\VmCmos.bin";
    memset(p->CMOS, 0, sizeof(p->CMOS));

    // Load in CMOS resource

    hInstance = AfxGetResourceHandle();
    hFound = FindResource(hInstance, MAKEINTRESOURCE(IDR_CMOS), "BINARY");
    ASSERT(hFound);
    hRes = LoadResource(hInstance, hFound);
    ASSERT(hRes);
    dwResourceSize = SizeofResource(hInstance,hFound);
    ASSERT(dwResourceSize==sizeof(p->CMOS));
    lpBuff = LockResource(hRes);
    memcpy(p->CMOS, lpBuff, dwResourceSize);

    // Create a temp file for the virtual floppy

    //    p->Disk[0].Flags = DSK_ENABLED | DSK_FLOPPY | DSK_VIRTUAL;
    //    p->Disk[0].PathToFile = _tempnam("C:\\", "VmDisk");
    //p->Disk[0].PathToFile = "C:\\VmFloppy.dsk";

    // Load the floppy file resource into the write cache
    //    p->Disk[1].Flags = DSK_FLOPPY;

    return TRUE;
}


BOOL CVMguiDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    if (!CDocument::OnOpenDocument(lpszPathName))
        return FALSE;

    // use the document as a configuration profile
    CString section = _T("Machine");
    TCHAR data[16384];
    DWORD nSize = sizeof(data) / sizeof(TCHAR);
    // read all items in one section
    DWORD oSize = GetPrivateProfileSection(section, data, nSize, lpszPathName);
    if (oSize == nSize - 2)
    {
        // buffer was too small for all
        ASSERT(0);
    }
    TCHAR *pline;
    PVMCONFIG p = &pTheApp->m_VMConfig;

    // Zero out the existing configuration data

    pTheApp->VmConfigClear();

    // Fill up the data from the config file

    for (pline = data, nSize = 0; *pline != _T('\0') && nSize < oSize; )
    {
        CString cs = pline;
        if (int n = cs.Find(_T('=')))
        {
            CString key = cs.Left(n);
            CString keydata = cs.Right(cs.GetLength() - (n + 1));

            if (key == _T("PhysMem"))
                p->PhysMem = _tcstoul(keydata, NULL, 10);

            if (key == _T("PathToCMOS"))
                p->PathToCMOS = keydata;

            CString ident;
            for( int d=0; d<MAX_DISKS; d++)
            {
                PDISKCONFIG pD = &pTheApp->m_VMConfig.Disk[d];

                ident.Format(_T("Disk%d_Flags"), d);
                if (key == ident)
                    pD->Flags = _tcstoul(keydata, NULL, 10);

                ident.Format(_T("Disk%d_PathToFile"), d);
                if (key == ident)
                    pD->PathToFile = keydata;

                ident.Format(_T("Disk%d_tracks"), d);
                if (key == ident)
                    pD->tracks = _tcstoul(keydata, NULL, 10);

                ident.Format(_T("Disk%d_heads"), d);
                if (key == ident)
                    pD->heads = _tcstoul(keydata, NULL, 10);

                ident.Format(_T("Disk%d_sectors"), d);
                if (key == ident)
                    pD->sectors = _tcstoul(keydata, NULL, 10);
            }
        }
        nSize += _tcslen(pline) + 1;
        pline += _tcslen(pline) + 1;
    }

    // Calculate max_sectors field
    pTheApp->VmConfigCalcSectors();

    // Load CMOS data

    FILE *fp;
    fp = fopen(pTheApp->m_VMConfig.PathToCMOS, "rb");
    if(fp)
    {
        fread(pTheApp->m_VMConfig.CMOS, sizeof(pTheApp->m_VMConfig.CMOS), 1, fp);
        fclose(fp);
    }
    else
    {
        // Cannot open CMOS data file to read
        CString Msg("Unable to open CMOS data file:\r");

        MessageBox(NULL, Msg + pTheApp->m_VMConfig.PathToCMOS, "Error", MB_OK);
    }

    // Calculate extended memory size for CMOS

    pTheApp->m_VMConfig.CMOS[0x30] = (BYTE) ((p->PhysMem - 1) * 1024) & 0xFF;          // Low byte
    pTheApp->m_VMConfig.CMOS[0x31] = (BYTE)(((p->PhysMem - 1) * 1024) >> 8) & 0xFF;    // High byte

    return TRUE;
}

static void BuildSection(DWORD *len, TCHAR *data, TCHAR *dline)
{
    _tcscpy(&data[*len], dline);
    *len += _tcslen(dline) + 1;
}

BOOL CVMguiDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
    CString section = _T("Machine");
    TCHAR data[1024];
    DWORD oSize = 0;
    TCHAR pline[128];
    // all items in one section are written at once

    // physmem
    _stprintf(pline, _T("PhysMem=%d"), pTheApp->m_VMConfig.PhysMem);
    BuildSection(&oSize, data, pline);

    // PathToCMOS
    _stprintf(pline, _T("PathToCMOS=%s"), pTheApp->m_VMConfig.PathToCMOS);
    BuildSection(&oSize, data, pline);

    // Save CMOS data

    FILE *fp;
    fp = fopen(pTheApp->m_VMConfig.PathToCMOS, "wb");
    if(fp)
    {
        fwrite(pTheApp->m_VMConfig.CMOS, sizeof(pTheApp->m_VMConfig.CMOS), 1, fp);
        fclose(fp);
    }
    else
    {
        // Cannot open CMOS data file to save
        CString Msg("Unable to save CMOS data file:\r");

        MessageBox(NULL, Msg + pTheApp->m_VMConfig.PathToCMOS, "Error", MB_OK);
    }

    // Save information about disks
    PVMCONFIG p = &pTheApp->m_VMConfig;

    for( int n=0; n<MAX_DISKS; n++ )
    {
        PDISKCONFIG p = &pTheApp->m_VMConfig.Disk[n];

        _stprintf(pline, _T("Disk%d_Flags=%d"), n, p->Flags);
        BuildSection(&oSize, data, pline);

        _stprintf(pline, _T("Disk%d_PathToFile=%s"), n, p->PathToFile);
        BuildSection(&oSize, data, pline);

        if( !(p->Flags & DSK_FLOPPY) )
        {
            _stprintf(pline, _T("Disk%d_tracks=%d"), n, p->tracks);
            BuildSection(&oSize, data, pline);

            _stprintf(pline, _T("Disk%d_heads=%d"), n, p->heads);
            BuildSection(&oSize, data, pline);

            _stprintf(pline, _T("Disk%d_sectors=%d"), n, p->sectors);
            BuildSection(&oSize, data, pline);
        }
    }

    // example of another line
    //  BuildSection(&oSize, data, _T("Floppy=3.5-144"));
    data[oSize] = _T('\0');

    BOOL b = WritePrivateProfileSection(section, data, lpszPathName);

    SetModifiedFlag(FALSE);

    return b;
}

