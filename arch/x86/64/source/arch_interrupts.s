; ok, this is our main interrupt handling stuff
BITS 64

%macro pushAll 0
  push rsp
  push rax
  push rcx
  push rdx
  push rbx
  push rbp
  push rsi
  push rdi
  push r8
  push r9
  push r10
  push r11
  push r12
  push r13
  push r14
  push r15
  mov ax, es
  push rax
  mov ax, ds
  push rax
%endmacro

%macro popAll 0
  pop rax
  mov ds, ax
  pop rax
  mov es, ax
  pop r15
  pop r14
  pop r13
  pop r12
  pop r11
  pop r10
  pop r9
  pop r8
  pop rdi
  pop rsi
  pop rbp
  pop rbx
  pop rdx
  pop rcx
  pop rax
  pop rsp
%endmacro

%define KERNEL_DS 0x20

%macro changeData 0
        mov ax, KERNEL_DS
        mov ss, ax
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
%endmacro

extern arch_saveThreadRegisters

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
        mov rdi, rsp
        mov rsi, 0
        call arch_saveThreadRegisters
        call irqHandler_%1
        popAll
        iretq
%endmacro

%macro dummyhandler  1
global arch_dummyHandler_%1
extern dummyHandler_%1
arch_dummyHandler_%1:
        pushAll
        call dummyHandler_%1
        popAll
        iretq
%endmacro

%macro errorhandler  1
global arch_errorHandler_%1
extern errorHandler_%1
arch_errorHandler_%1:
        pushAll
        call errorHandler_%1
        popAll
        iretq
%endmacro

section .text

extern pageFaultHandler
global arch_pageFaultHandler
arch_pageFaultHandler:
        pushAll ; pushes 144 bytes to stack
        changeData
        mov rdi, rsp
        mov rsi, 1
        call arch_saveThreadRegisters
        mov rsi, [rsp + 144]
        mov rdi, cr2
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
    mov rdi, rsp
    mov rsi, 0
    call arch_saveThreadRegisters
    call syscallHandler
    hlt
