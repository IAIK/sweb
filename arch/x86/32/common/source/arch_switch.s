
BITS 32

section .text

extern g_tss;

extern currentThreadInfo
extern blubbabla;

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
        ;mov esp, dword[ebx + 28]     ; restore esp
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
    ;lock     ;         # next instruction is locked
    ;according to the intel manuals xchg is already locked (fixed a warning)
    xchg  dword[edx], eax;, # swap %eax with what is stored in (%edx)
                     ;  # ... and don't let any other cpu touch that
                      ; # ... memory location while you're swapping
    ret               ; # return the old value that's in %eax
