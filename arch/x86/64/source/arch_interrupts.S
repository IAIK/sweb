/* https://www.airs.com/blog/archives/518 */
.section .note.GNU-stack,"",@progbits

/* ok, this is our main interrupt handling stuff */

.code64
.text


/* #include <arch_interrupts.h> */

.equ KERNEL_DS, 0x20

.macro swapgsIfPrivChange cs_offset=8
    /*
#.if \errorcode
#    testl $3, 16(%rsp)
#.else
#    testl $3, 8(%rsp)
#.endif
    */
    testl $3, \cs_offset(%rsp)
    jz 1f

    /*
    # # debug
    # movw $0xE9, %dx
    # xorq %rax, %rax
    # movb 8(%rsp), %al
    # andl $3, %eax
    # addl $0x41, %eax
    # outb %al, %dx
    # hlt
    */

    swapgs
1:
.endm

.macro loadKernelSegments
  movw $KERNEL_DS, %ax
  movw %ax,%ss
  movw %ax,%ds
  movw %ax,%es
  /* movw %ax,%fs */
  /* movw %ax,%gs */
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

  /* push fs base */
  movq $0xC0000100, %rcx
  rdmsr
  pushq %rdx
  pushq %rax

  loadKernelSegments
.endm

.macro popAll

  /* pop fs base (only relevant when there's no contextSwitch) */
  movq $0xC0000100, %rcx
  popq %rax
  popq %rdx

  /* Only restore fs base when returning to user mode (user fs is thread specific, kernel fs is _CPU_ specific and changes depending on which cpu the thread is scheduled on, so the saved fs base is incorrect when the thread is re-scheduled on another cpu) */
  /* Test for user mode by looking at the privilege level of the data segment selector that's next on the stack */
  testl $3, (%rsp)
  jz 1f
  wrmsr
1:

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

.macro interrupthandler handlerfunction id
        swapgsIfPrivChange
	    pushAll

	    movq %rsp, %rdi
	    movq $0, %rsi

	    movq 160(%rsp), %rax /* rip at interrupt, depends on size of saved registers! */

	    /* set up fake call stack for debugger backtrace */
	    pushq %rax /* fake return address = rip at interrupt */
.global arch_irqHandler_backtrace_fix_\id
.type   arch_irqHandler_backtrace_fix_\id, @function
arch_irqHandler_backtrace_fix_\id:
	    pushq %rbp
	    movq %rsp, %rbp

	    call arch_saveThreadRegisters

	    call \handlerfunction

        leave
        popq %rdx /* pop simulated return value */

	    popAll
	    swapgsIfPrivChange
	    iretq
.endm

.macro irqhandler num
.global arch_irqHandler_\num
.type   arch_irqHandler_\num, @function
.extern irqHandler_\num
arch_irqHandler_\num:
        interrupthandler irqHandler_\num \num
.endm

/* TODO: not thread safe with multiple CPUs */
dummyhandlerscratchvariable:
  .long 0
  .long 0

.extern dummyHandler
.global arch_dummyHandler
.type 	arch_dummyHandler, @function
arch_dummyHandler:
.rept 128
        decq dummyhandlerscratchvariable
.endr
.extern dummyHandlerMiddle
.global arch_dummyHandlerMiddle
.type 	arch_dummyHandlerMiddle, @function
arch_dummyHandlerMiddle:
        swapgsIfPrivChange
        pushAll
        movq %rsp,%rdi
        movq $0,%rsi
        call arch_saveThreadRegisters
        movq $1,%rcx
        movq 168(%rsp),%rdx
        movq 160(%rsp), %rsi
        movq dummyhandlerscratchvariable,%rdi
        addq $128, %rdi
        movq $0,dummyhandlerscratchvariable
        call errorHandler
        popAll
        swapgsIfPrivChange
        iretq
        hlt


.extern errorHandler
.altmacro
.macro errorhandler num, errorcode=0
.global arch_errorHandler_\num
.type 	arch_errorHandler_\num, @function
arch_errorHandler_\num:
        swapgsIfPrivChange (8 + \errorcode*8)
        pushAll
        movq %rsp, %rdi
        /* movq $\errorcode, %rsi */
        movq $0, %rsi

        call arch_saveThreadRegisters

        movq $0, %rcx
    .if \errorcode
        movq 178(%rsp), %rdx /* arg3 = cs */
	    movq 168(%rsp), %rsi /* arg2 = rip */
    .else
        movq 168(%rsp), %rdx /* arg3 = cs */
        movq 160(%rsp), %rsi /* arg2 = rip */
    .endif
        movq $\num, %rdi     /* arg1 = num */

        /* set up fake call stack for debugger backtrace */
        pushq %rsi /* fake return address = rip at fault */
.global arch_errorHandler_backtrace_fix_\num
.type   arch_errorHandler_backtrace_fix_\num, @function
arch_errorHandler_backtrace_fix_\num:
        pushq %rbp
        movq %rsp, %rbp

        call errorHandler

        leave
        popq %rdx /* pop simulated return value */

        popAll
    .if \errorcode
        addq $8, %rsp
    .endif
        swapgsIfPrivChange
        iretq
        hlt
.endm


.text

.extern pageFaultHandler
.global arch_pageFaultHandler
.type   arch_pageFaultHandler, @function
arch_pageFaultHandler:
        swapgsIfPrivChange 16
        pushAll
        movq %rsp, %rdi
        /* movq $1, %rsi */
        movq $0, %rsi
        call arch_saveThreadRegisters
        movq 168(%rsp), %rdx /* arg3 = rip at pagefault */
        movq 160(%rsp), %rsi /* arg2 = error code */
        movq %cr2, %rdi      /* arg1 = pagefault address */

        /* set up fake call stack for debugger backtrace */
        pushq %rdx /* fake return address = rip at pagefault */
.global arch_pageFaultHandler_backtrace_fix
.type   arch_pageFaultHandler_backtrace_fix, @function
arch_pageFaultHandler_backtrace_fix:
        pushq %rbp
        movq %rsp, %rbp

        call pageFaultHandler

        leave
        popq %rdx

        popAll
        addq $8, %rsp
        swapgsIfPrivChange
        iretq
        hlt

.macro interrupt_entrystub errorcode num
    .global arch_interrupt_entrystub_\num
    .type   arch_interrupt_entrystub_\num, @function
arch_interrupt_entrystub_\num\():
.if \errorcode == 0
    pushq $0
.endif
    pushq $\num
    jmp arch_interruptHandlerGeneric
.endm

.extern genericInterruptHandler
.global arch_interruptHandlerGeneric
.type   arch_interruptHandlerGeneric, @function
arch_interruptHandlerGeneric:
    swapgsIfPrivChange 24
	pushAll

	movq %rsp, %rdi

	movq 176(%rsp), %rax /* rip at interrupt, depends on size of saved registers! */

	/* set up fake call stack for debugger backtrace */
	pushq %rax /* fake return address = rip at interrupt */
.global arch_interruptHandlerGeneric_backtrace_fix
.type   arch_interruptHandlerGeneric_backtrace_fix, @function
arch_interruptHandlerGeneric_backtrace_fix:
	pushq %rbp
	movq %rsp, %rbp

    call genericInterruptEntry

    leave
    popq %rdx /* pop simulated return address */

	popAll
    addq $16, %rsp /* pop interrupt number and error code */

	swapgsIfPrivChange
	iretq
    hlt


.altmacro
.macro gen_entrystub__ has_errorcode:req start:req end:req
    counter=\start
    .rept \end - \start
        interrupt_entrystub \has_errorcode %counter
        counter=counter+1
    .endr
.endm

/* half open range [start, end) */
.altmacro
.macro gen_entrystub has_errorcode:req start:req end
    counter=\start
    end__=\start+1
    .ifnb \end
        end__=\end
    .endif
    gen_entrystub__ \has_errorcode \start %end__
    /*
    # .rept \end - \start
    #     interrupt_entrystub \has_errorcode %counter
    #     counter=counter+1
    # .endr
    */
.endm

gen_entrystub 0 0 8
gen_entrystub 1 8
gen_entrystub 0 9
gen_entrystub 1 10 15
gen_entrystub 0 15 17
gen_entrystub 1 17
gen_entrystub 0 18 21
gen_entrystub 1 21
gen_entrystub 0 22 29
gen_entrystub 1 29 31
gen_entrystub 0 31
gen_entrystub 0 32 256

.macro gen_vector_ptr num
    .quad "arch_interrupt_entrystub_\num"
.endm

.altmacro
.macro gen_vector_table count
    counter=0
    .rept \count
        gen_vector_ptr %counter
        counter=counter+1
.endr
.endm

.global generated_idt_vector_table
generated_idt_vector_table:
gen_vector_table 256

.irp num,0,1,3,4,6,9,11,14,15,65,90,91,100,101,127
irqhandler \num
.endr

.irp num,8,10,11,12,13,14,17
errorhandler \num, 1
.endr

.irp num,0,4,5,6,7,9,16,18,19
errorhandler \num
.endr

.global arch_syscallHandler
.type   arch_syscallHandler, @function
.extern syscallHandler
arch_syscallHandler:
        interrupthandler syscallHandler syscall
