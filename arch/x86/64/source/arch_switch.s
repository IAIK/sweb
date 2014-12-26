
section .text
BITS 64

extern g_tss;

;;;;;; this one is different from the mona approach
;;;;;; they have one global pointer to the current thread
;;;;;; we however have two of them, one to the thread
;;;;;; and one to the thread info
;;;;;; This is way more robust against changes in the thread class
extern currentThreadInfo
global currentThreadInfo

;;----------------------------------------------------------------------
;; swtich thread to user and change page
;;----------------------------------------------------------------------
global arch_switchThreadToUserPageDirChange
arch_switchThreadToUserPageDirChange:
        mov rbx, currentThreadInfo
        mov rbx, [rbx]
        frstor [rbx + 224]
        mov rcx, g_tss        ; tss
        mov rax, qword[rbx + 200]     ; get rsp0
        mov qword[rcx + 4], rax      ; restore rsp0
        mov rax, qword[rbx + 216]     ; page directory
        mov cr3, rax                 ; change page directory pointer table
        mov rax, qword[rbx + 24]     ; restore rax
        mov rcx, qword[rbx + 32]     ; restore rcx
        mov rdx, qword[rbx + 40]     ; restore rdx
        ;mov rsp, qword[rbx + 56]     ; restore rsp
        mov rbp, qword[rbx + 64]     ; restore rbp
        mov rsi, qword[rbx + 72]     ; restore rsi
        mov rdi, qword[rbx + 80]     ; restore rdi
        mov r8, qword[rbx + 88]      ; restore r8
        mov r9, qword[rbx + 96]      ; restore r9
        mov r10, qword[rbx + 104]    ; restore r10
        mov r11, qword[rbx + 112]    ; restore r11
        mov r12, qword[rbx + 120]    ; restore r12
        mov r13, qword[rbx + 128]    ; restore r13
        mov r14, qword[rbx + 136]    ; restore r14
        mov r15, qword[rbx + 144]    ; restore r15
        mov ds , word[rbx + 152]      ; restore ds
        mov es , word[rbx + 152]      ; restore es
        mov fs , word[rbx + 152]      ; restore fs
        mov gs , word[rbx + 152]      ; restore gs

        push qword[rbx + 152]          ; push ss
        push qword[rbx + 56]          ; push rsp
        push qword[rbx + 16]          ; push rflags
        push qword[rbx + 8]          ; push cs
        push qword[rbx + 0]          ; push rip

        mov rbx, qword[rbx + 48]     ; restore rbx

        iretq                        ; switch to next

;;----------------------------------------------------------------------
;; switch thread and change page
;;----------------------------------------------------------------------
global arch_switchThreadKernelToKernelPageDirChange
arch_switchThreadKernelToKernelPageDirChange:
        mov rbx, currentThreadInfo
        mov rbx, [rbx]
        frstor [rbx + 224]
        mov rcx, g_tss        ; tss
        mov rax, qword[rbx + 200]     ; get rsp0
        mov qword[rcx + 4], rax      ; restore rsp0
        mov rax, qword[rbx + 216]     ; page directory
        mov cr3, rax                 ; change page directory pointer table
        mov rax, qword[rbx + 24]     ; restore rax
        mov rcx, qword[rbx + 32]     ; restore rcx
        mov rdx, qword[rbx + 40]     ; restore rdx
        mov rsp, qword[rbx + 56]     ; restore rsp
        mov rbp, qword[rbx + 64]     ; restore rbp
        mov rsi, qword[rbx + 72]     ; restore rsi
        mov rdi, qword[rbx + 80]     ; restore rdi
        mov r8, qword[rbx + 88]      ; restore r8
        mov r9, qword[rbx + 96]      ; restore r9
        mov r10, qword[rbx + 104]    ; restore r10
        mov r11, qword[rbx + 112]    ; restore r11
        mov r12, qword[rbx + 120]    ; restore r12
        mov r13, qword[rbx + 128]    ; restore r13
        mov r14, qword[rbx + 136]    ; restore r14
        mov r15, qword[rbx + 144]    ; restore r15
        mov ds , word[rbx + 152]      ; restore ds
        mov es , word[rbx + 160]      ; restore es
        push qword[rbx + 184]          ; push ss
        push qword[rbx + 56]          ; push rsp
        push qword[rbx + 16]          ; push rflags
        push qword[rbx + 8]          ; push cs
        push qword[rbx + 0]          ; push rip

        mov rbx, qword[rbx + 48]

        iretq                        ; switch to next

global arch_TestAndSet
  arch_TestAndSet:
    mov rax, rdi      ; get new_value into %rax
    xchg  [rsi], rax  ; swap %rax with what is stored in (%rdx)
                      ; ... and don't let any other cpu touch that
                      ; ... memory location while you're swapping
    ret               ; return the old value that's in %rax

