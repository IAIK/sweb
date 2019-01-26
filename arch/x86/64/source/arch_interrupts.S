# ok, this is our main interrupt handling stuff

.code64
.text

.equ KERNEL_DS, 0x20

.macro swapgsIfPrivChange
    testl $3, 8(%rsp)
    jz 1f

    # debug
    #movw $0xE9, %dx
    #xorq %rax, %rax
    #movb 8(%rsp), %al
    #andl $3, %eax
    #addl $0x41, %eax
    #outb %al, %dx
    #hlt

    swapgs
1:
.endm

.macro swapgsIfPrivChangeErrorCode
    testl $3, 16(%rsp)
    jz 1f

    # debug
    #movw $0xE9, %dx
    #xorq %rax, %rax
    #movb 16(%rsp), %al
    #andl $3, %eax
    #addl $0x41, %eax
    #outb %al, %dx
    #hlt

    swapgs
1:
.endm

.macro loadKernelSegments
  movw $KERNEL_DS, %ax
  movw %ax,%ss
  movw %ax,%ds
  movw %ax,%es
  #movw %ax,%fs
  #movw %ax,%gs
.endm

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

  loadKernelSegments
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
        swapgsIfPrivChange
        pushAll
        movq %rsp,%rdi
        movq $0,%rsi
        call arch_saveThreadRegisters
        call irqHandler_\num
        popAll
        swapgsIfPrivChange
        iretq
.endm

dummyhandlerscratchvariable:
  .long 0
  .long 0

.extern dummyHandler
.global arch_dummyHandler
arch_dummyHandler:
.rept 128
        decq dummyhandlerscratchvariable
.endr
.extern dummyHandlerMiddle
.global arch_dummyHandlerMiddle
arch_dummyHandlerMiddle:
        swapgsIfPrivChange
        pushAll
        movq %rsp,%rdi
        movq $0,%rsi
        call arch_saveThreadRegisters
        movq $1,%rcx
        movq 152(%rsp),%rdx
        movq 144(%rsp), %rsi
        movq dummyhandlerscratchvariable,%rdi
        addq $128, %rdi
        movq $0,dummyhandlerscratchvariable
        call errorHandler
        popAll
        swapgsIfPrivChange
        iretq
        hlt


.extern errorHandler
.macro errorhandler num
.global arch_errorHandler_\num
arch_errorHandler_\num:
        swapgsIfPrivChange
        pushAll
        movq %rsp,%rdi
        movq $0,%rsi
        call arch_saveThreadRegisters
        movq $0,%rcx
        movq 152(%rsp),%rdx  # rip
        movq 144(%rsp), %rsi # error code
        movq $\num,%rdi
        call errorHandler
        popAll
        swapgsIfPrivChange
        iretq
        hlt
.endm

.macro errorhandlerWithCode num
.global arch_errorHandler_\num
arch_errorHandler_\num:
        swapgsIfPrivChangeErrorCode
        pushAll
        movq %rsp,%rdi
        movq $1,%rsi
        call arch_saveThreadRegisters
        movq $0,%rcx
        movq 160(%rsp),%rdx
        movq 152(%rsp), %rsi
        movq $\num,%rdi
        call errorHandler
        popAll
        addq $8,%rsp
        swapgsIfPrivChangeErrorCode
        iretq
        hlt
.endm

.text

.extern pageFaultHandler
.global arch_pageFaultHandler
arch_pageFaultHandler:
        swapgsIfPrivChangeErrorCode
        pushAll
        movq %rsp, %rdi
        movq $1, %rsi
        call arch_saveThreadRegisters
        movq 152(%rsp), %rdx # rip
        movq 144(%rsp), %rsi # error code
        movq %cr2, %rdi
        call pageFaultHandler
        popAll
        addq $8, %rsp
        swapgsIfPrivChangeErrorCode
        iretq
        hlt


.irp num,0,1,3,4,6,9,11,14,15,65,90,99,100
irqhandler \num
.endr

.irp num,8,10,11,12,13,14,17
errorhandlerWithCode \num
.endr

.irp num,0,4,5,6,7,9,16,18,19
errorhandler \num
.endr

.global arch_syscallHandler
.extern syscallHandler
arch_syscallHandler:
    swapgsIfPrivChange
    pushAll
    movq %rsp,%rdi
    movq $0,%rsi
    call arch_saveThreadRegisters
    call syscallHandler
    hlt
