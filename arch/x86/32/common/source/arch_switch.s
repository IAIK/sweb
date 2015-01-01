
BITS 32

section .text

extern g_tss;

global arch_contextSwitchToUser
arch_contextSwitchToUser:
        mov ebx, dword[esp + 4]
        push dword[ebx + 60]         ; push ss  here dpl lowwer
        push dword[ebx + 28]         ; push esp here dpl lowwer
        jmp arch_contextSwitch
global arch_contextSwitchToKernel
arch_contextSwitchToKernel:
        mov ebx, dword[esp + 4]
        mov esp, dword[ebx + 28]     ; restore esp
global arch_contextSwitch
arch_contextSwitch:
        frstor [ebx + 80]
        mov ecx, dword[g_tss]        ; tss
        mov eax, dword[ebx + 68]     ; get esp0
        mov dword[ecx + 4], eax      ; restore esp0
        mov eax, dword[ebx + 76]     ; page directory        
        mov cr3, eax                 ; change page directory
        mov eax, dword[ebx + 12]     ; restore eax
        mov ecx, dword[ebx + 16]     ; restore ecx
        mov edx, dword[ebx + 20]     ; restore edx
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
