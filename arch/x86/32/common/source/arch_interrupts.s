

; ok, this is our main interrupt handling stuff
BITS 32

%define KERNEL_DS 0x10

%macro pushAll 0
        pushad
        push ds
        push es
%endmacro

%macro popAll 0
        pop es
        pop ds
        popad
%endmacro

%macro changeData 0
        mov ax, KERNEL_DS
        mov es, ax
        mov ax, KERNEL_DS
        mov ds, ax
%endmacro


extern arch_saveThreadRegisters
extern arch_saveThreadRegistersForPageFault

; ; ; 
; macros for irq and int handlers
; ; ;
;;; IRQ handlers
%macro irqhandler 1
global arch_irqHandler_%1
extern irqHandler_%1
arch_irqHandler_%1:
        pushAll
        changeData
        push ebp
        mov  ebp, esp
        push 0
        call arch_saveThreadRegisters
        leave
        call irqHandler_%1
        popAll
        iretd
%endmacro

%macro dummyhandler  1
global arch_dummyHandler_%1
extern dummyHandler_%1
arch_dummyHandler_%1:
        pushAll
        changeData
        call dummyHandler_%1
        popAll
        iretd
%endmacro

%macro errorhandler  1
global arch_errorHandler_%1
extern errorHandler_%1
arch_errorHandler_%1:
        pushAll
        changeData
        call errorHandler_%1
        popAll
        iretd
%endmacro

section .text

extern pageFaultHandler
global arch_pageFaultHandler
arch_pageFaultHandler:
        ;we are already on a new stack because a privliedge switch happened
        pushAll
        changeData
        push ebp
        mov  ebp, esp
        push 1
        call arch_saveThreadRegisters
        leave
        push ebp
        mov  ebp, esp
        sub  esp, 8
        mov  eax, dword[esp + 52] ; error cd
        mov  dword[esp + 4], eax
        mov  eax, cr2             ; page fault address
        mov  dword[esp + 0], eax
        call pageFaultHandler
        hlt

%assign i 0
%rep 16
irqhandler i
%assign i i+1
%endrep
  
irqhandler 65
  
errorhandler 0
dummyhandler 1
dummyhandler 2
dummyhandler 3
errorhandler 4
errorhandler 5
errorhandler 6
errorhandler 7
errorhandler 8
errorhandler 9
errorhandler 10
errorhandler 11
errorhandler 12
errorhandler 13
;dummyhandler 14
dummyhandler 15
errorhandler 16
errorhandler 17
errorhandler 18
errorhandler 19

%assign i 20
%rep 108 ; generate dummyhandler 20-128
dummyhandler i
%assign i i+1
%endrep

global arch_syscallHandler
extern syscallHandler
arch_syscallHandler:
    pushAll
    changeData
    push ebp
    mov  ebp, esp
    push 0
    call arch_saveThreadRegisters
    leave
    call syscallHandler
    hlt
