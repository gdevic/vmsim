/******************************************************************************
*                                                                             *
*   Module:     Globals.h                                                     *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       3/16/2000                                                     *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains all the global function declarations and
        inline function definintions as well as declarations of global data

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 3/16/2000  Original                                             Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Include Files                                                             *
******************************************************************************/
#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include "DeviceExtension.h"

/******************************************************************************
*                                                                             *
*   Declaration of Global Data                                                *
*                                                                             *
******************************************************************************/

extern PDEVICE_OBJECT GUIDevice;// Our user-inteface device object

extern TVM      *pVM;           // Pointer to VM data structure
extern TMonitor *pMonitor;      // Pointer to Monitor data structure

extern TMemory  Mem;            // Driver memory management structure
extern TCPU     CPU;            // Virtual CPU data structure
extern TDevice  Device;         // Virtual devices structures

/******************************************************************************
*                                                                             *
*   Declaration of Global Functions                                           *
*                                                                             *
******************************************************************************/

//-----------------------------------------------------------------------------
// Driver.c
//-----------------------------------------------------------------------------
extern void SendRequestToGui(PVMREQUEST pVmReq);
extern void OutputMessageToGUI(PDEVICE_EXTENSION pDevExt, char *mssg);

//-----------------------------------------------------------------------------
// TaskBridge.c
//-----------------------------------------------------------------------------
extern void TaskSwitch();
extern int TaskBridgeToVM(void);
extern int TaskBridgeToHost(void);

//-----------------------------------------------------------------------------
// MainLoop.c
//-----------------------------------------------------------------------------
typedef void (*TPostState)(void);
enum { stateNext=0, stateStep, stateNative, stateIoCtl };
extern void SetMainLoopState( int new_state, TPostState funct );
extern void RunVM(DWORD param);

//-----------------------------------------------------------------------------
// CPU.c
//-----------------------------------------------------------------------------
extern void SetCpuMode( DWORD CpuMode );
#define CPU_REAL_MODE       0   //  real mode
#define CPU_V86_MODE        1   //  virtual-86 mode (under a VM kernel monitor)
#define CPU_KERNEL_MODE     2   //  kernel Ring 0 [1] protected mode
#define CPU_USER_MODE       3   //  protected Ring 3 user mode

extern void CPU_Reset();
extern WORD CPU_PopWord(void);
extern DWORD CPU_PopDword(void);
extern int CPU_PushWord(WORD wWord);
extern int CPU_PushDword(DWORD dwDword);
extern int CPU_Interrupt( BYTE bIntNumber );
extern int CPU_Exception( BYTE bException, DWORD ErrorCode );
#define EXCEPTION_DIVIDE_ERROR            0
#define EXCEPTION_DEBUG_EXCEPTION         1
#define EXCEPTION_NMI                     2
#define EXCEPTION_BREAKPOINT              3
#define EXCEPTION_INTO                    4
#define EXCEPTION_BOUND                   5
#define EXCEPTION_INVALID_OPCODE          6
#define EXCEPTION_COPROCESSOR_NA          7
#define EXCEPTION_DOUBLE_FAULT            8
#define EXCEPTION_COPROCESSOR_SEGO        9
#define EXCEPTION_INVALID_TSS             10
#define EXCEPTION_SEG_NOT_PRESENT         11
#define EXCEPTION_STACK_FAULT             12
#define EXCEPTION_GPF                     13
#define EXCEPTION_PAGE_FAULT              14
#define EXCEPTION_RESERVED_15             15
#define EXCEPTION_COPROCESSOR_ERROR       16

extern void TranslateVmGdtToMonitorGdt();
extern void TranslateSingleVmGdtToMonitorGdt( DWORD dwSel );
extern void LoadCR( DWORD cr, DWORD value );

//-----------------------------------------------------------------------------
// Initialization.c
//-----------------------------------------------------------------------------
extern DWORD GetGDTBase( TGDT_Gate *pGDT);
extern DWORD GetGDTLimit( TGDT_Gate *pGDT);
extern DWORD GetGDTBaseLimit( DWORD *pLimit, TGDT_Gate *pGDT);
extern void SetIDT(TIDT_Gate *pIDT, DWORD selector, DWORD offset, int type);
extern void SetGDT(TGDT_Gate *pGDT, DWORD base, DWORD limit, int type, BOOL f32);
extern void RebaseMonitor();
extern int InitVM();
extern void ShutDownVM();

//-----------------------------------------------------------------------------
// CPUTraps.c
//-----------------------------------------------------------------------------
extern int ExecuteTrap( TMeta *pMeta, int nInt );

//-----------------------------------------------------------------------------
// ExternalInterrupts.c
//-----------------------------------------------------------------------------
extern void ExecuteINTn( int nInt );

//-----------------------------------------------------------------------------
// Scanner.c
//-----------------------------------------------------------------------------
extern int ScanCodePage( BYTE *pCSIP, BYTE *pMetaPage, DWORD Mode );
extern void DisassembleCSIP();

//-----------------------------------------------------------------------------
// Meta.c
//-----------------------------------------------------------------------------
extern void MetaInitialize();
extern TMeta * SetupMetaPage( DWORD dwVmCSIP );
extern void MetaInvalidatePage(TMeta *pMeta);
extern void MetaInvalidateAll();
extern void MetaInvalidateCurrent();
extern BOOL MetaInvalidateVmPhysAddress( DWORD dwVmPhys );
extern BOOL MetaConditionallyInvalidateCode( TMeta *pMeta, DWORD dwOffset );

//-----------------------------------------------------------------------------
// DisassemblerForVirtualization.c
//-----------------------------------------------------------------------------
extern int DisVirtualize( BYTE *bpTarget, DWORD VmLinear, TMEM *pMem );

//-----------------------------------------------------------------------------
// MemoryAllocation.c
//-----------------------------------------------------------------------------
extern void MemoryAllocInit();
extern DWORD MemoryAlloc(DWORD dwSize, DWORD *pUser, BOOL fLocked);
extern int MemoryFree( DWORD Address );
extern int AllocVMMemory();
extern void MemoryMapROM( PVOID pRom, DWORD dwSize, DWORD dwVmPhysical);
extern int MemoryCheck();

//-----------------------------------------------------------------------------
// MemoryAccess.c
//-----------------------------------------------------------------------------
extern void InitMemoryAccess();
extern TPostState MemoryAccess( BYTE access, BYTE data_len, TMEM *pMem );

//-----------------------------------------------------------------------------
// SystemRegions.c
//-----------------------------------------------------------------------------
extern void InitSysReg();
typedef void (*TSysReg_Callback)(void);
TSysReg_Callback SysRegFind( DWORD dwVmLin, DWORD dwVmPage );
extern void RenewSystemRegion( int eSysReg, DWORD dwVmAddress, DWORD dwLen );
extern void AddSystemRegionPTE( DWORD dwPhys );
extern void DeleteSystemRegion( int eSysReg );
extern void DeleteSystemRegionPTE( DWORD dwVmPhys );

//-----------------------------------------------------------------------------
// VmPaging.c
//-----------------------------------------------------------------------------
extern TPage *BuildMonitorMapping( DWORD dwVmLin );
extern void EnablePaging();
extern void DisablePaging();
extern void A20SetState(BOOL fEnabled);

//-----------------------------------------------------------------------------
// Translations.c
//-----------------------------------------------------------------------------
extern DWORD CSIPtoVmLinear();
extern DWORD CSIPtoDriverLinear();
extern DWORD VmLinearToDriverLinear( DWORD dwVmLin );
extern DWORD VmPageIndexToNtPageIndex( DWORD VmIndex );
extern DWORD AnyNtVirtualToPhysical( DWORD dwAddress );
extern TPage *VmLinearToVmPte( DWORD dwVmLin );

//=============================================================================
// REGISTRATION FUNCTIONS
//=============================================================================
extern void IoRegister(TVDD *pVdd);
extern void MemoryRegister(TVDD *pVdd);
extern void PciRegister(TVDD *pVdd);
extern void PIIX4_Register(TVDD *pVdd);
extern void CmosAndRtc_Register(TVDD *pVdd);
extern void Pic_Register(TVDD *pVdd);
extern void Pit_Register(TVDD *pVdd);
extern void Dma_Register(TVDD *pVdd);
extern void Keyboard_Register(TVDD *pVdd);
extern void Vga_Register(TVDD *pVdd);
extern void Mda_Register(TVDD *pVdd);
extern void Floppy_Register(TVDD *pVdd);
extern void Hdd_Register(TVDD *pVdd);

//-----------------------------------------------------------------------------
// Exported from virtual devices
//-----------------------------------------------------------------------------
extern BYTE Pic_Ack();
extern DWORD DummyIOPortIN(DWORD port, DWORD io_len);
extern void DummyIOPortOUT(DWORD port, DWORD data, DWORD io_len);

extern DWORD Dma_GetAddress(DWORD channel);
extern DWORD Dma_GetOutstanding(DWORD channel);
extern BOOL Dma_IsTC(DWORD channel);
extern void Dma_Roll(DWORD channel, WORD advance);
extern void Dma_Next(DWORD channel);

enum eSource { SOURCE_KEYBOARD=0, SOURCE_PS2, SOURCE_OUTPORT };
extern void ControllerEnq(BYTE value, int eSource);


//=============================================================================
// Inline assembly functions that are just much simpler to do this way
//=============================================================================

_inline WORD Get_CS()       { _asm xor eax, eax _asm mov ax, cs }
_inline WORD Get_DS()       { _asm xor eax, eax _asm mov ax, ds }
_inline WORD Get_ES()       { _asm xor eax, eax _asm mov ax, es }
_inline WORD Get_FS()       { _asm xor eax, eax _asm mov ax, fs }
_inline WORD Get_GS()       { _asm xor eax, eax _asm mov ax, gs }
_inline WORD Get_SS()       { _asm xor eax, eax _asm mov ax, ss }
_inline WORD Get_EFLAGS()   { _asm pushfd _asm pop eax }
_inline WORD Get_ESP()      { _asm mov eax, esp }

_inline int Get_CR3_Index() { _asm mov eax, cr3 _asm shr eax, 12 }
_inline int Get_CR4()       { _asm _emit 0x0F _asm _emit 0x20 _asm _emit 0xE0 }
_inline int Set_CR4()       { _asm _emit 0x0F _asm _emit 0x22 _asm _emit 0xE0 }

_inline WORD Get_LDT()
{
    WORD _ldt;
    _asm sldt word ptr [_ldt]
    return( _ldt );
}

_inline void GetCPUID(int _eax, DWORD *p)
{
    _asm pushad
    _asm mov    eax, [_eax]
    _asm mov    edi, [p]
    _asm cpuid
    _asm mov    [edi], eax
    _asm mov    [edi+4], ebx
    _asm mov    [edi+8], ecx
    _asm mov    [edi+12], edx
    _asm popad
}

_inline int CpuHasCPUID()
{
    int value = 0;

    _asm    pushad
    _asm    pushfd
    _asm    or      dword ptr ss:[esp], 1 shl ID_BIT
    _asm    popfd
    _asm    pushfd
    _asm    bt      dword ptr ss:[esp], ID_BIT
    _asm    jnc     no_support
    _asm    and     dword ptr ss:[esp], Not (1 shl ID_BIT)
    _asm    popfd
    _asm    pushfd
    _asm    bt      dword ptr ss:[esp], ID_BIT
    _asm    jc      no_support
    _asm    mov     [value], 1
no_support:
    _asm    popfd
    _asm    popad

    return value;
}


#endif //  _GLOBALS_H_
