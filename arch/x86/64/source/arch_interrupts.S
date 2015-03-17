# ok, this is our main interrupt handling stuff

.code64
.text

.equ KERNEL_DS, 0x20

.macro pushAll
  pushq %rsp
  pushq %rax
  pushq %rcx
  pushq %rdx
  pushq %rbx
  pushq %rbp
  pushq %rsi
  pushq %rdi
  pushq %r8
  pushq %r9
  pushq %r10
  pushq %r11
  pushq %r12
  pushq %r13
  pushq %r14
  pushq %r15
  movw %es,%ax
  pushq %rax
  movw %ds,%ax
  pushq %rax
  movw $KERNEL_DS, %ax
  movw %ax,%ss
  movw %ax,%ds
  movw %ax,%es
  movw %ax,%fs
  movw %ax,%gs
.endm

.macro popAll
  popq %rax
  movw %ax,%ds
  popq %rax
  movw %ax,%es
  popq %r15
  popq %r14
  popq %r13
  popq %r12
  popq %r11
  popq %r10
  popq %r9
  popq %r8
  popq %rdi
  popq %rsi
  popq %rbp
  popq %rbx
  popq %rdx
  popq %rcx
  popq %rax
  popq %rsp
.endm

.extern arch_saveThreadRegisters

.macro irqhandler num
.global arch_irqHandler_\num
.extern irqHandler_\num
arch_irqHandler_\num:
        pushall
        movq %rsp,%rdi
        movq $0,%rsi
        call arch_saveThreadRegisters
        call irqHandler_\num
        popall
        iretq
.endm

.global arch_dummyHandler
.extern dummyHandler
arch_dummyHandler:
        pushall
        call dummyHandler
        popall
        iretq

.macro errorhandler num
.global arch_errorHandler_\num
.extern errorHandler_\num
arch_errorHandler_\num:
        pushall
        call errorHandler_\num
        popall
        iretq
.endm

.text

.extern pageFaultHandler
.global arch_pageFaultHandler
arch_pageFaultHandler:
        pushall
        movq %rsp,%rdi
        movq $1,%rsi
        call arch_saveThreadRegisters
        movq 144(%rsp),%rsi
        movq %cr2, %rdi
        call pageFaultHandler
        popall
        addq $8,%rsp
        iretq
        hlt


.irp num,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,65
irqhandler \num
.endr

.irp num,0,4,5,6,7,8,9,10,11,12,13,16,17,18,19
errorhandler \num
.endr

.global arch_syscallHandler
.extern syscallHandler
arch_syscallHandler:
    pushall
    movq %rsp,%rdi
    movq $0,%rsi
    call arch_saveThreadRegisters
    call syscallHandler
    hlt
