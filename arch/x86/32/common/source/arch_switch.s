
BITS 32

section .text

global arch_contextSwitchToUser
arch_contextSwitchToUser:
		mov ecx, dword[esp + 12]     ; local core bss address
		mov edx, dword[esp + 8]      ; local core lastThreadState Address
		mov ebx, dword[esp + 4]      ; local core currentThread Address
		
		mov eax, dword[ebx + 68]	 ; get esp0
		mov word[eax], gs			 ; write gs to kernel stack
		
        push dword[ebx + 60]         ; push ss  here dpl lowwer
        push dword[ebx + 28]         ; push esp here dpl lowwer
        
        jmp arch_contextSwitch
        
global arch_contextSwitchToKernel
arch_contextSwitchToKernel:
		mov ecx, dword[esp + 12]     ; local core bss address
        mov edx, dword[esp + 8]      ; local core lastThreadState Address
		mov ebx, dword[esp + 4]      ; local core currentThread Address
		
        mov esp, dword[ebx + 28]     ; restore esp
global arch_contextSwitch
arch_contextSwitch:
        frstor [ebx + 80]
        mov eax, dword[ebx + 68]     ; get esp0
        mov dword[ecx + 4], eax      ; restore esp0
        mov eax, dword[ebx + 76]     ; page directory        
        mov cr3, eax                 ; change page directory
        mov eax, dword[ebx + 12]     ; restore eax
        mov ecx, dword[ebx + 16]     ; restore ecx
        mov ebp, dword[ebx + 32]     ; restore ebp
        mov esi, dword[ebx + 36]     ; restore esi
        mov edi, dword[ebx + 40]     ; restore edi
        mov es , word[ebx + 48]      ; restore es
        mov ds , word[ebx + 44]      ; restore ds
        push dword[ebx + 8]          ; push eflags
        push dword[ebx + 4]          ; push cs
        push dword[ebx + 0]          ; push eip
        
        push dword[ebx + 24]		 ; push ebx
        
        push edx					 ; push lastThreadStateAddress
        mov edx, dword[ebx + 20]     ; restore edx
        pop ebx						 ; pop lastThreadStateAddress
        ;;;check if ebx == 0 ----> don't set lastThreadState
        cmp ebx, 0h
        je finish_switch
		cmp dword[ebx], 0h			 ; compare lastThreadState with 0
        jne finish_switch			 ; if != 0 --> finished
        mov dword[ebx], 3h			 ; set lastThreadState to 'Ready'
   finish_switch:
        pop  ebx                     ; restore ebx
        iretd                        ; switch to next
        