# ok, this is our main interrupt handling stuff

.code32
.text

.equ KERNEL_DS, 0x10
.equ KERNEL_FS, 0x38
.equ KERNEL_GS, 0x38

.macro loadKernelSegments
  movw $KERNEL_DS, %ax
  movw %ax,%es
  movw %ax,%ds
  movw $KERNEL_FS, %ax
  movw %ax,%fs
  movw $KERNEL_GS, %ax
  movw %ax,%gs
.endm

.macro pushAll
  pushal
  push %ds
  push %es
  push %fs
  push %gs
  loadKernelSegments
.endm

.macro popAll
  pop %gs
  pop %fs
  pop %es
  pop %ds
  popal
.endm

.extern arch_saveThreadRegisters

.macro interrupthandler handlerfunction
  pushall
  pushl %esp
  pushl $0
  call arch_saveThreadRegisters
  addl $8, %esp
  call \handlerfunction
  popall
  iretl
.endm

.macro irqhandler num
.global arch_irqHandler_\num
.extern irqHandler_\num
.stabs "arch_irqHandler_\num()",36,0,0,arch_irqHandler_\num
arch_irqHandler_\num:
  interrupthandler irqHandler_\num
.endm

dummyhandlerscratchvariable:
  .long 0
  .long 0

.extern dummyHandler
.global arch_dummyHandler
arch_dummyHandler:
.rept 128
        call arch_dummyHandlerMiddle
.endr
.extern dummyHandlerMiddle
.global arch_dummyHandlerMiddle
arch_dummyHandlerMiddle:
        pushall

	    pushl %esp
        pushl $0
        call arch_saveThreadRegisters
        addl $8, %esp

        pushl 48(%esp) # eip
        call arch_computeDummyHandler # retval in eax
        addl $4, %esp

        movl 48(%esp), %ebx # eip
        movl 52(%esp), %ecx # cs
        pushl $1
        pushl %ecx
        pushl %ebx
        pushl %eax
        call errorHandler
        addl $12, %esp

        popall
        addl $4,%esp
        iretl
        hlt

.extern errorHandler
.macro errorhandler num, errorcode=0
.global arch_errorHandler_\num
arch_errorHandler_\num:
        pushall

        pushl %esp
        pushl $\errorcode
        call arch_saveThreadRegisters
        addl $8, %esp

    .if \errorcode
    	movl 52(%esp), %eax # eip
	    movl 56(%esp), %ebx # cs
    .else
    	movl 48(%esp), %eax # eip
	    movl 52(%esp), %ebx # cs
    .endif
        pushl $0
        pushl %ebx
        pushl %eax
        pushl $\num
        call errorHandler
        addl $16, %esp

        popall
    .if \errorcode
    	addl $4,%esp
    .endif
        iretl
        hlt
.endm

.extern pageFaultHandler
.global arch_pageFaultHandler
.stabs "arch_pageFaultHandler()",36,0,0,arch_pageFaultHandler
arch_pageFaultHandler:
  #we are already on a new stack because a priviledge switch happened
  pushall

  pushl %esp
  pushl $1
  call arch_saveThreadRegisters
  addl $8, %esp

  movl 52(%esp), %eax # eip
  movl 48(%esp), %ebx # error code
  movl %cr2, %ecx     # pagefault address
  pushl %eax
  pushl %ebx
  pushl %ecx
  call pageFaultHandler
  addl $12, %esp

  popall
  addl $4,%esp
  iretl
  hlt


.irp num,0,1,3,4,6,9,11,14,15,65,90,99,100
irqhandler \num
.endr

.irp num,8,10,11,12,13,14,17
errorhandler \num, 1
.endr

.irp num,0,4,5,6,7,9,16,18,19
errorhandler \num
.endr

.global arch_syscallHandler
.extern syscallHandler
.stabs "arch_syscallHandler",36,0,0,arch_syscallHandler
arch_syscallHandler:
  interrupthandler syscallHandler
