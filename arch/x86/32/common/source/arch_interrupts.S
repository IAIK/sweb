# ok, this is our main interrupt handling stuff

.code32
.text

.equ KERNEL_DS, 0x10

.macro pushAll
  pushal
  push %ds
  push %es
  movw $KERNEL_DS, %ax
  movw %ax,%es
  movw $KERNEL_DS, %ax
  movw %ax,%ds
.endm

.macro popAll
  pop %es
  pop %ds
  popal
.endm

.extern arch_saveThreadRegisters
.extern arch_saveThreadRegistersForPageFault

.macro irqhandler num
.global arch_irqHandler_\num
.extern irqHandler_\num
arch_irqHandler_\num:
  pushall
  pushl %ebp
  movl %esp,%ebp
  pushl $0
  call arch_saveThreadRegisters
  leave
  call irqHandler_\num
  popall
  iretl
.endm

.global arch_dummyHandler
.extern dummyHandler
arch_dummyHandler:
  pushall
  call dummyHandler
  popall
  iretl

.macro errorhandler num
.global arch_errorHandler_\num
.extern errorHandler_\num
arch_errorHandler_\num:
  pushall
  call errorHandler_\num
  popall
  iretl
.endm

.extern pageFaultHandler
.global arch_pageFaultHandler
arch_pageFaultHandler:
  #we are already on a new stack because a privliedge switch happened
  pushall
  pushl %ebp
  movl %esp,%ebp
  pushl $1
  call arch_saveThreadRegisters
  leave
  pushl %ebp
  movl %esp,%ebp
  subl $8,%esp
  movl 52(%esp),%eax        # error cd
  movl %eax, 4(%esp)
  movl %cr2, %eax           # page fault address
  movl %eax, 0(%esp)
  call pageFaultHandler
  leave
  popall
  addl $4,%esp
  iretl
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
  pushl %ebp
  movl %esp,%ebp
  pushl $0
  call arch_saveThreadRegisters
  leave
  call syscallHandler
  hlt
