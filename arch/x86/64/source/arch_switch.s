
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

%macro store_general_regs 0
        mov rax, qword [rsp + 104]; save rbp
        mov qword[rbx + 64], rax
        mov rax, qword [rsp + 136]; save rax
        mov qword[rbx + 24], rax
        mov rax, qword [rsp + 128]; save rcx
        mov qword[rbx + 32], rax
        mov rax, qword [rsp + 120]; save rdx
        mov qword[rbx + 40], rax
        mov rax, qword [rsp + 112]; save rbx
        mov qword[rbx + 48], rax
        mov rax, qword [rsp + 96]; save rsi
        mov qword[rbx + 72], rax
        mov rax, qword [rsp + 88]; save rdi
        mov qword[rbx + 80], rax
        mov rax, qword [rsp + 80]; save r8
        mov qword[rbx + 88], rax
        mov rax, qword [rsp + 72]; save r9
        mov qword[rbx + 96], rax
        mov rax, qword [rsp + 64]; save r10
        mov qword[rbx + 104], rax
        mov rax, qword [rsp + 56]; save r11
        mov qword[rbx + 112], rax
        mov rax, qword [rsp + 48]; save r12
        mov qword[rbx + 120], rax
        mov rax, qword [rsp + 40]; save r13
        mov qword[rbx + 128], rax
        mov rax, qword [rsp + 32]; save r14
        mov qword[rbx + 136], rax
        mov rax, qword [rsp + 24]; save r15
        mov qword[rbx + 144], rax
        mov rax, [rsp + 16]      ; save ds
        mov qword[rbx + 152], rax
        mov rax, [rsp + 8]       ; save es
        mov qword[rbx + 160], rax
%endmacro

global arch_saveThreadRegisters
arch_saveThreadRegisters:
        mov rbx, currentThreadInfo
        mov rbx, [rbx]
        fnsave [rbx + 224]
        frstor [rbx + 224]
        mov rax, qword[rsp + 160]  ; get cs
        and rax, 0x03             ; check cpl is 3
        cmp rax, 0x03
        je .from_user
.from_kernel:
        mov rax, qword [rsp + 176]; save rsp
        mov qword[rbx + 56], rax
        mov rax, qword [rsp + 152]; save rip
        mov qword[rbx], rax
        mov rax, qword [rsp + 160]; save cs
        mov qword[rbx + 8], rax
        mov rax, qword [rsp + 168]; save rflags
        mov qword[rbx + 16], rax
        store_general_regs
        ret
.from_user:
        mov rax, qword[rsp + 176] ; save rsp0
        mov qword[rbx + 56], rax
        mov rax, qword [rsp + 152]; save rip
        mov qword[rbx], rax
        mov rax, qword [rsp + 160]; save cs
        mov qword[rbx + 8], rax
        mov rax, qword [rsp + 168]; save rflags
        mov qword[rbx + 16], rax
        store_general_regs
        ret

global arch_saveThreadRegistersForPageFault
arch_saveThreadRegistersForPageFault:
        mov rbx, currentThreadInfo
        mov rbx, [rbx]
        fnsave [rbx + 224]
        frstor [rbx + 224]
        mov rax, qword[rsp + 168]  ; get cs
        and rax, 0x03             ; check cpl is 3
        cmp rax, 0x03
        je .from_user
.from_kernel:
        mov rax, qword [rsp + 184]; save rsp
        mov qword[rbx + 56], rax
        mov rax, qword [rsp + 160]; save rip
        mov qword[rbx], rax
        mov rax, qword [rsp + 168]; save cs
        mov qword[rbx + 8], rax
        mov rax, qword [rsp + 176]; save rflags
        mov qword[rbx + 16], rax
        store_general_regs
        ret
.from_user:
        mov rax, qword[rsp + 184] ; save rsp0
        mov qword[rbx + 56], rax
        mov rax, qword [rsp + 160]; save rip
        mov qword[rbx], rax
        mov rax, qword [rsp + 168]; save cs
        mov qword[rbx + 8], rax
        mov rax, qword [rsp + 176]; save rflags
        mov qword[rbx + 16], rax
        store_general_regs
        ret

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

