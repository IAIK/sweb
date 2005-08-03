
BITS 32

section .text

extern g_tss;

;;;;;; this one is different from the mona approach
;;;;;; they have one global pointer to the current thread
;;;;;; we however have two of them, one to the thread
;;;;;; and one to the thread info
;;;;;; This is way more robust against changes in the thread class
extern currentThreadInfo
global arch_saveThreadRegisters
arch_saveThreadRegisters:
;        mov eax, dword[currentThreadInfo]
;        mov ebx, dword[eax + 0 ]  ; ArchThreadInfo
        mov ebx, dword[currentThreadInfo]
        fnsave [ebx + 80]
        frstor [ebx + 80]
        mov eax, dword[esp + 48]  ; get cs
        and eax, 0x03             ; check cpl is 3
        cmp eax, 0x03
        je from_user
from_kernel:
        mov eax, dword [esp + 44]; save eip
        mov dword[ebx], eax
        mov eax, dword [esp + 48]; save cs
        mov dword[ebx + 4], eax
        mov eax, dword [esp + 52]; save eflags
        mov dword[ebx + 8], eax
        mov eax, dword [esp + 40]; save eax
        mov dword[ebx + 12], eax
        mov eax, dword [esp + 36]; save ecx
        mov dword[ebx + 16], eax
        mov eax, dword [esp + 32]; save edx
        mov dword[ebx + 20], eax
        mov eax, dword [esp + 28]; save ebx
        mov dword[ebx + 24], eax
        mov eax, dword [esp + 24]; save esp
        add eax, 0xc
        mov dword[ebx + 28], eax
        mov eax, dword [esp + 20]; save ebp
        mov dword[ebx + 32], eax
        mov eax, dword [esp + 16]; save esi
        mov dword[ebx + 36], eax
        mov eax, dword [esp + 12]; save edi
        mov dword[ebx + 40], eax
        mov eax, [esp + 8]      ; save ds
        mov dword[ebx + 44], eax
        mov eax, [esp + 4]      ; save es
        mov dword[ebx + 48], eax
        ret
from_user:
        mov eax, dword[esp + 60] ; save ss3
        mov dword[ebx + 60], eax
        mov eax, dword[esp + 56] ; save esp3
        mov dword[ebx + 28], eax
        mov eax, dword [esp + 44]; save eip
        mov dword[ebx], eax
        mov eax, dword [esp + 48]; save cs
        mov dword[ebx + 4], eax
        mov eax, dword [esp + 52]; save eflags
        mov dword[ebx + 8], eax
        mov eax, dword [esp + 40]; save eax
        mov dword[ebx + 12], eax
        mov eax, dword [esp + 36]; save ecx
        mov dword[ebx + 16], eax
        mov eax, dword [esp + 32]; save edx
        mov dword[ebx + 20], eax
        mov eax, dword [esp + 28]; save ebx
        mov dword[ebx + 24], eax
        mov eax, dword [esp + 20]; save ebp
        mov dword[ebx + 32], eax
        mov eax, dword [esp + 16]; save esi
        mov dword[ebx + 36], eax
        mov eax, dword [esp + 12]; save edi
        mov dword[ebx + 40], eax
        mov eax, [esp + 8]      ; save ds
        mov dword[ebx + 44], eax
        mov eax, [esp + 4]      ; save es
        mov dword[ebx + 48], eax
        ret

global arch_switchThreadKernelToKernel
arch_switchThreadKernelToKernel:
;        mov eax, dword[currentThreadInfo]
;        mov ebx, dword[eax + 0 ]  ; ArchThreadInfo
        mov ebx, dword[currentThreadInfo]
        frstor [ebx + 80]
        mov ecx, dword[g_tss]        ; tss
        mov eax, dword[ebx + 68]     ; get esp0
        mov dword[ecx + 4], eax      ; restore esp0
        mov eax, dword[ebx + 12]     ; restore eax
        mov ecx, dword[ebx + 16]     ; restore ecx
        mov edx, dword[ebx + 20]     ; restore edx
        mov esp, dword[ebx + 28]     ; restore esp
        mov ebp, dword[ebx + 32]     ; restore ebp
        mov esi, dword[ebx + 36]     ; restore esi
        mov edi, dword[ebx + 40]     ; restore edi
        mov es , word[ebx + 48]      ; restore es
        mov ds , word[ebx + 44]      ; restore ds
        push dword[ebx + 8]          ; push eflags
        push dword[ebx + 4]          ; push cs
        push dword[ebx + 0]          ; push eip
        push dword[ebx + 24]
        pop  ebx                     ; restore ebx
        iretd                        ; switch to next
        
;;----------------------------------------------------------------------
;; swtich thread to user and change page
;;----------------------------------------------------------------------
global arch_switchThreadToUserPageDirChange
arch_switchThreadToUserPageDirChange:
;        mov eax, dword[g_currentThread]
;        mov ebx, dword[eax + 0 ]     ; ArchThreadInfo
        mov ebx, dword[currentThreadInfo]
        frstor [ebx + 80]

        mov ecx, dword[g_tss]        ; tss
        mov eax, dword[ebx + 68]     ; get esp0
        mov dword[ecx + 4], eax      ; restore esp0
        mov eax, dword[ebx + 76]     ; page directory
        mov cr3, eax                 ; change page directory

       mov eax, dword[ebx + 12]     ; restore eax
        mov ecx, dword[ebx + 16]     ; restore ecx
        mov edx, dword[ebx + 20]     ; restore edx
        mov esp, dword[ebx + 28]     ; restore esp
        mov ebp, dword[ebx + 32]     ; restore ebp
        mov esi, dword[ebx + 36]     ; restore esi
        mov edi, dword[ebx + 40]     ; restore edi
        mov es , word[ebx + 48]      ; restore es
        mov ds , word[ebx + 44]      ; restore ds
        push dword[ebx + 60]         ; push ss  here dpl lowwer
        push dword[ebx + 28]         ; push esp here dpl lowwer
        push dword[ebx + 8]          ; push eflags
        push dword[ebx + 4]          ; push cs
        push dword[ebx + 0]          ; push eip
        push dword[ebx + 24]
 
        pop  ebx                     ; restore ebp
        iretd                        ; switch to next

;;----------------------------------------------------------------------
;; switch thread and change page
;;----------------------------------------------------------------------
global arch_switchThreadKernelToKernelPageDirChange
arch_switchThreadKernelToKernelPageDirChange:
;        mov eax, dword[g_currentThread]
;        mov ebx, dword[eax + 0 ]     ; ArchThreadInfo
        mov ebx, dword[currentThreadInfo]
        frstor [ebx + 80]
        mov ecx, dword[g_tss]        ; tss
        mov eax, dword[ebx + 68]     ; get esp0
        mov dword[ecx + 4], eax      ; restore esp0
        mov eax, dword[ebx + 76]     ; page directory        
        mov cr3, eax                 ; change page directory
        mov eax, dword[ebx + 12]     ; restore eax
        mov ecx, dword[ebx + 16]     ; restore ecx
        mov edx, dword[ebx + 20]     ; restore edx
        mov esp, dword[ebx + 28]     ; restore esp
        mov ebp, dword[ebx + 32]     ; restore ebp
        mov esi, dword[ebx + 36]     ; restore esi
        mov edi, dword[ebx + 40]     ; restore edi
        mov es , word[ebx + 48]      ; restore es
        mov ds , word[ebx + 44]      ; restore ds
        push dword[ebx + 8]          ; push eflags
        push dword[ebx + 4]          ; push cs
        push dword[ebx + 0]          ; push eip
        push dword[ebx + 24]
        pop  ebx                     ; restore ebx
        iretd                        ; switch to next

global arch_TestAndSet
  arch_TestAndSet:
    mov eax, dword[esp+4];,%eax  # get new_value into %eax
    mov edx, dword[esp+8]; 8(%esp),%edx  # get lock_pointer into %edx
    lock     ;         # next instruction is locked
    xchg  dword[edx], eax;, # swap %eax with what is stored in (%edx)
                     ;  # ... and don't let any other cpu touch that
                      ; # ... memory location while you're swapping
    ret               ; # return the old value that's in %eax


global arch_restoreUserThreadRegisters
  arch_restoreUserThreadRegisters:
    mov ebx, dword[currentThreadInfo]
    frstor [ebx + 80]
    
    mov ecx, dword[g_tss]        ; tss
    mov eax, dword[ebx + 68]     ; get esp0
    mov dword[ecx + 4], eax      ; restore esp0
;    mov eax, dword[ebx + 76]     ; page directory
;    mov cr3, eax                 ; change page directory
    
    mov eax, dword[ebx + 12]     ; restore eax
    mov ecx, dword[ebx + 16]     ; restore ecx
    mov edx, dword[ebx + 20]     ; restore edx
    mov esp, dword[ebx + 28]     ; restore esp
    mov ebp, dword[ebx + 32]     ; restore ebp
    mov esi, dword[ebx + 36]     ; restore esi
    mov edi, dword[ebx + 40]     ; restore edi
    mov es , word[ebx + 48]      ; restore es
    mov ds , word[ebx + 44]      ; restore ds
    push dword[ebx + 60]         ; push ss  here dpl lowwer
    push dword[ebx + 28]         ; push esp here dpl lowwer
    push dword[ebx + 8]          ; push eflags
    push dword[ebx + 4]          ; push cs
    push dword[ebx + 0]          ; push eip
    push dword[ebx + 24]
    
    pop  ebx                     ; restore ebp
    ret
