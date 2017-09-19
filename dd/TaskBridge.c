/******************************************************************************
*                                                                             *
*   Module:     TaskBridge.c                                                  *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       3/14/2000                                                     *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the code for the VM interrupt handling
        including a TaskBridge function.

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 3/14/2000  Original                                             Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Include Files                                                             *
******************************************************************************/
#include <strmini.h>                // Include driver level C functions

#include <ntddk.h>                  // Include ddk function header file

#include "Vm.h"                     // Include root header file

/******************************************************************************
*                                                                             *
*   Local Defines, Variables and Macros                                       *
*                                                                             *
******************************************************************************/

/******************************************************************************
*                                                                             *
*   Functions                                                                 *
*                                                                             *
******************************************************************************/

/******************************************************************************
*                                                                             *
*   void TaskSwitch()                                                         *
*                                                                             *
*******************************************************************************
*
*   Switch to a VM task.  The task will run until interrupted by either
*   an external interrupt or internal (to CPU) trap, whose number is then
*   stored in pMonitor->nInterrupt field.
*
******************************************************************************/
void TaskSwitch()
{
    // Make VM task TSS selector not busy so we can load it

    pMonitor->GDT[ VM_SEL_TSS >> 3 ].type = DESC_TYPE_TSS32A;

    _asm
    {
        mov     ebx, [pMonitor]                     // Get to the monitor data

        pushfd                                      // Store flags
        pushad                                      // Store all registers
        cli                                         // Disable interrupts

        ;===========================================
        ; Setting up working control registers
        ;===========================================
        mov     eax, cr0                            // Get host CR0 register
        push    eax                                 // Store it on the stack
        lea     edi, [CPU]
        mov     eax, [edi + TCPU.cr0]               // Get the VM percieved cr0 register
        and     eax, 0x0004002F                     // Let AM NE TS EM MP be settable by a vm
        or      eax, 0x80010011                     // +PE +ET +WP +PG
        mov     cr0, eax                            // Load it into the effective CR0

        mov_eax_cr4                                 // Get host CR4 register
        push    eax                                 // Store it on the stack
        mov     eax, [ebx + TMonitor.guestCR4]      // and reload faked CR4
        mov_cr4_eax

        mov     ecx, [ebx + TMonitor.offPD_Inject]  // Where to inject our bridge page
        add     ecx, NT_PAGE_DIR                    // NT 5 Page Directory
        mov     eax, [ebx + TMonitor.VMPDphysical]  // What page to inject
        xchg    eax, [ecx]                          // Insert bridge into our address space
        mov     edx, cr3
        mov     cr3, edx                            // Invalidate TLB
                                                     // try 'invlpg' instead
        // Store the host state of the processor

        sgdt    fword ptr [ebx + TMonitor.hostGDT]
        sidt    fword ptr [ebx + TMonitor.hostIDT]

        push    ecx                                 // Store PD address
        push    eax                                 // Store original PD page
        push    ebx
        call    fword ptr [ebx + TMonitor.fn_TaskBridgeToVM]
        pop     ebx

        // An active TSS in NT5 has busy bit set; we need to reset it in order for
        // ltr instruction not to gp fault

        mov     eax, NT_SEL_TSS                     // Reload task register
        mov     edx, [ebx + TMonitor.hostGDT.base]
        and     byte ptr [eax + edx + 5], Not 2     // so, make it not busy
        ltr     ax                                  // this will mark it busy again

        xor     ax, ax
        lldt    ax                                  // TODO: NT5 does not use LDT

        pop     eax                                 // Restore original page
        pop     ecx                                 // Restore address
        mov     [ecx], eax                          // restore PD entry
        mov     eax, cr3
        mov     cr3, eax                            // Invalidate TLB

        pop     eax                                 // Restore cr4 from the stack
        mov_cr4_eax

        pop     eax                                 // Restore cr0 from the stack
        mov     cr0, eax

        mov     eax, cr2                            // Get the possible page fault address
        mov     [ebx + TMonitor.guestCR2], eax      // Store it for the case of PF

        popad                                       // Restore all registers
        popfd                                       // Restore flags
    }
}


/******************************************************************************
*                                                                             *
*   TaskBridgeToVM()                                                          *
*                                                                             *
*******************************************************************************
*
*   This function is copied in the shared pages that are accessible both
*   within the host and VM address space at the same linear address.
*
*   The pages are injected into the host address space at the time
*   of a jump, and restored on the exit from VM, done by a caller to this
*   function - TaskSwitch().
*
*   The pages are injected into the guest address space in the RebaseMonitor()
*   function that sets the monitor position-dependent code at the init time
*   and every time when the monitor gets moved.
*
*   On entry:
*      ebx - Monitor structure in driver address space
*
******************************************************************************/
__declspec( naked ) TaskBridgeToVM()
{
    _asm
    {
        // Store the stack pointer for the re-entry into the driver

        mov     ds:[ebx + TMonitor.DriverESP], esp
        mov     esp, ds:[ebx + TMonitor.guestTSS.esp0]  ; Use new ESP

        lgdt    fword ptr [ebx + TMonitor.guestGDT] ; Load GDT register
        lidt    fword ptr [ebx + TMonitor.guestIDT] ; Load IDT register
        lldt    [ebx + TMonitor.guestLDT]           ; Load LDT register

        mov     eax, [ebx + TMonitor.guestCR3]      ; Get the new CR3 value
        mov     cr3, eax                            ; Bang! We are in the new address space!
        ; This move invalidated TLB cache as well   !

        mov     ax, VM_SEL_MONITOR_DS               ; Get the new DS selector value
        mov     ds, ax                              ; Reload DS within new address space
        mov     ss, ax                              ; Move SS to within monitor space, too

        mov     ax, VM_SEL_TSS                      ; Load VM TSS selector from the new GDT
        ltr     ax                                  ; this will also mark it busy (in GDT)

        test    ds:[TMonitor.guestTSS.eflags], TF_MASK + INTERNAL_RUN_NATIVE_MASK
        jnz     SkipTlbPreload

        ; The basic idea is as follows:
        ;
        ;   * Write RETF using private mapping
        ;   * Call RETF instruction using normal mapping (Loads instruction TLB entry)
        ;   * Restore RETF byte using private mapping
        ;   * Set PT entry to Ring-0 accessible
        ;   * Switch to User mode code using normal mapping

        ; Since we are still running with the NTs code selector, not the monitors one,
        ; RETF would fault unless we change the returning selector to VM_SEL_MONITOR_CS
        ;
        ; Little code stub that we set with a dword, and call it, looks like:
        ;
        ; 59  pop  ecx        ; Pop the returning address value (eip) and discard
        ; 59  pop  edx        ; Pop the returning code selector (NT_SEL_CS) and discard
        ; CB  retf            ; Use previously stored returning address and selector
        ;
        mov     eax, 090CB5959h                     ; Returning code stub

        lea     ebx, ds:[TMonitor.MonCECP]
        xchg    ds:[ebx], eax                       ; Write ret. stub via private mapping

        push    dword ptr VM_SEL_MONITOR_CS         ; Push our CS on the stack

        mov     edx, offset ReturnFromStub
        sub     edx, offset TaskBridgeToVM
        add     edx, MONITOR_BRIDGE_OFFSET
        push    edx                                 ; Push new returning offset

        call    fword ptr ds:[TMonitor.fn_CECP]     ; Call the stub via global mapping
ReturnFromStub:

        mov     ds:[ebx], eax                       ; Restore using private mapping

        mov     eax, ds:[TMonitor.offPTE_CECP]      ; Get to the PTE for that page
        and     dword ptr ds:[eax], Not 1           ; Make CECP page not present

SkipTlbPreload:
        and     ds:[TMonitor.guestTSS.eflags], Not INTERNAL_RUN_NATIVE_MASK
        test    ds:[TMonitor.guestTSS.eflags], VM_MASK
        jz      GuestRunsPM

;GuestRunsV86:
        push    dword ptr ds:[TMonitor.guestTSS.gs]
        push    dword ptr ds:[TMonitor.guestTSS.fs]
        push    dword ptr ds:[TMonitor.guestTSS.ds]
        push    dword ptr ds:[TMonitor.guestTSS.es]
        push    dword ptr ds:[TMonitor.guestTSS.ss]
        push    dword ptr ds:[TMonitor.guestTSS.esp]
        push    dword ptr ds:[TMonitor.guestTSS.eflags]
        push    dword ptr ds:[TMonitor.guestTSS.cs]
        push    dword ptr ds:[TMonitor.guestTSS.eip]

        mov     eax, ds:[TMonitor.guestTSS.eax]
        mov     ecx, ds:[TMonitor.guestTSS.ecx]
        mov     edx, ds:[TMonitor.guestTSS.edx]
        mov     ebx, ds:[TMonitor.guestTSS.ebx]

        mov     ebp, ds:[TMonitor.guestTSS.ebp]
        mov     esi, ds:[TMonitor.guestTSS.esi]
        mov     edi, ds:[TMonitor.guestTSS.edi]

        iretd

GuestRunsPM:

        push    dword ptr ds:[TMonitor.guestTSS.ss]
        push    dword ptr ds:[TMonitor.guestTSS.esp]
        push    dword ptr ds:[TMonitor.guestTSS.eflags]
        push    dword ptr ds:[TMonitor.guestTSS.cs]
        push    dword ptr ds:[TMonitor.guestTSS.eip]
        push    dword ptr ds:[TMonitor.guestTSS.ds]

        mov     ax, ds:[TMonitor.guestTSS.gs]
        mov     gs, ax
        mov     ax, ds:[TMonitor.guestTSS.fs]
        mov     fs, ax
        mov     ax, ds:[TMonitor.guestTSS.es]
        mov     es, ax

        mov     eax, ds:[TMonitor.guestTSS.eax]
        mov     ecx, ds:[TMonitor.guestTSS.ecx]
        mov     edx, ds:[TMonitor.guestTSS.edx]
        mov     ebx, ds:[TMonitor.guestTSS.ebx]

        mov     ebp, ds:[TMonitor.guestTSS.ebp]
        mov     esi, ds:[TMonitor.guestTSS.esi]
        mov     edi, ds:[TMonitor.guestTSS.edi]

        pop     ds
        iretd

        ;======================================================================
        _emit 0x55                                  ; End-of-function signature
        _emit 0xAA                                  ;  for copying
    }
}


/******************************************************************************
*                                                                             *
*   TaskBridgeToHost()                                                        *
*                                                                             *
*******************************************************************************
*
*   This function is copied in the shared pages that are accessible both
*   within the host and VM address space at the same linear address.
*
*   This function is jumped from each interrupt handler snippet.
*
******************************************************************************/
__declspec( naked ) TaskBridgeToHost()
{
//  The interrupt stubs looks like the following:
//
//  Interrupt stubs for ints without error code on the ring-0 stack:
//
//  IntXX:  push    eax (dummy)
//          push    eax
//          mov     al, int#
//          jmp     TaskBridgeToHost
//
//  Interrupt stubs for ints that place an error code on the ring-0 stack:
//
//  IntXX:
//          push    eax
//          mov     al, int#
//          jmp     TaskBridgeToHost
//
    // On the ring-0 stack (from current TSS):
    //
    //    (I) Running V86 code:             (II) Running PM code:
    //   ___________________________        ______________________________
    //   esp from tss * -|-
    //              ---- | VM gs
    //              ---- | VM fs
    //              ---- | VM ds
    //              ---- | VM es             esp from tss * -|-
    //              ---- | VM ss                        ---- | PM ss
    //              VM esp                              PM esp
    //              VM eflags                           PM eflags
    //              ---- | VM cs                        ---- | PM cs
    //              VM eip                              PM eip
    //              error_code or dummy                 error_code or dummy
    //   ss:esp ->  eax                       ss:esp -> eax
    //   eax = int#                           eax = int#

    _asm
    {
        mov     byte ptr ss:[TMonitor.nInterrupt], al   ; Save interrupt number
        pop     dword ptr ss:[TMonitor.guestTSS.eax]    ; Save eax
        pop     dword ptr ss:[TMonitor.nErrorCode]      ; Save error code

        pop     dword ptr ss:[TMonitor.guestTSS.eip]    ; Save return address
        pop     dword ptr ss:[TMonitor.guestTSS.cs]     ; Save guest CS
        pop     eax
        mov     ss:[TMonitor.guestTSS.eflags], eax      ; Save eflags

        ; Depending on the guest mode (V86 or PM), we do different cleanup

        test    eax, VM_MASK                            ; Running in V86 mode ?
        jz      GuestWasRunningPM

;GuestWasRunningV86:

        mov     ax, VM_SEL_MONITOR_DS                   ; Now, get us to our data
        mov     ds, ax

        pop     dword ptr ds:[TMonitor.guestTSS.esp]    ; Save the rest of registers
        pop     dword ptr ds:[TMonitor.guestTSS.ss]
        pop     dword ptr ds:[TMonitor.guestTSS.es]
        pop     dword ptr ds:[TMonitor.guestTSS.ds]
        pop     dword ptr ds:[TMonitor.guestTSS.fs]
        pop     dword ptr ds:[TMonitor.guestTSS.gs]

        jmp     GuestCommonBack

GuestWasRunningPM:
        mov     ax, ds
        mov     ss:[TMonitor.guestTSS.ds], ax

        mov     ax, VM_SEL_MONITOR_DS                   ; Now, get us to our data
        mov     ds, ax

        pop     dword ptr ds:[TMonitor.guestTSS.esp]
        pop     dword ptr ds:[TMonitor.guestTSS.ss]

        mov     ax, es
        mov     ds:[TMonitor.guestTSS.es], ax
        mov     ax, fs
        mov     ds:[TMonitor.guestTSS.fs], ax
        mov     ax, gs
        mov     ds:[TMonitor.guestTSS.gs], ax

GuestCommonBack:

        mov     ds:[TMonitor.guestTSS.ecx], ecx     ; Save the rest of GP registers
        mov     ds:[TMonitor.guestTSS.edx], edx
        mov     ds:[TMonitor.guestTSS.ebx], ebx

        mov     ds:[TMonitor.guestTSS.ebp], ebp
        mov     ds:[TMonitor.guestTSS.esi], esi
        mov     ds:[TMonitor.guestTSS.edi], edi

        // -----------------------------------------+
        // We have all the registers saved now
        // -----------------------------------------+
        mov     esp, ds:[TMonitor.DriverESP]        ; Reload driver esp

        lgdt    fword ptr ds:[TMonitor.hostGDT]     ; Load GDT descriptor registers
        lidt    fword ptr ds:[TMonitor.hostIDT]     ; Load IDT descriptor registers

        mov     eax, ds:[TMonitor.hostCR3]          ; Get host CR3 value
        mov     cr3, eax                            ; Bang! We are in the host address space!

        mov     ax, NT_SEL_GS
        mov     gs, ax
        mov     ax, NT_SEL_FS
        mov     fs, ax
        mov     ax, NT_SEL_SS
        mov     ss, ax
        mov     ax, NT_SEL_ES
        mov     es, ax
        mov     ax, NT_SEL_DS
        mov     ds, ax

        retf                                        ; Restore CS as well...

        ;======================================================================
        ; Temporary helper code
cld
mov edi, 0x80000000 + 0xB0000 + 80*2
mov edx, ds:[ebp]
call PrintDWORD
mov edx, ds:[ebp+4]
call PrintDWORD
mov edx, ds:[ebp+8]
call PrintDWORD
mov edx, ds:[ebp+12]
call PrintDWORD

PrintDWORD:             ; edx - dword to print, edi - location
        push    eax
        push    ecx
        mov     ecx, 8
        mov     ah, 0x07
        rol     edx, 4
plp:    mov     al, dl
        and     al, 0x0F
        rol     edx, 4
        cmp     al, 9
        jle     not_ov
        add     al, 'A' - '9' - 1
not_ov: add     al, '0'
        stosw
        loop    plp
        inc     edi
        inc     edi
        pop     ecx
        pop     eax
        ret

mov al, 0xed
out 0x60, al
w_: in al, 0x64
test al, 2
jnz w_
mov al, 7
out 0x60, al

        _emit 0x55                                  ; End-of-function signature
        _emit 0xAA                                  ;  for copying
    }
}


