.text

.comm kernel_page_directory_start, 0x4000 @ Reserve 16k space for page directory

.bss
.comm stack, 0x4000 @ Reserve 4k stack in the BSS

.text

interrupt_vector_table:
.globl interrupt_vector_table
    ldr pc, =arch_irq0_handler @ Reset, supervisor
    ldr pc, =arch_irq1_handler @ Undefined instruction, undefined
    ldr pc, =arch_irq2_handler @ Software interrupt (SWI), supervisor
    ldr pc, =arch_irq3_handler @ Prefetch Abort (instruction fetch memory abort), abort
    ldr pc, =arch_irq4_handler @ Data Abort (data access memory abort), abort
    ldr pc, =arch_irq5_handler @ not used
    ldr pc, =arch_irq6_handler @ IRQ (interrupt), irq
    ldr pc, =arch_irq7_handler @ FIQ (fast interrupt, fiq

entry:
.globl entry
  ldr sp, =stack - 0x80000000+0x4000 @ Set up the stack
  mov fp, #0
  ldr r0, =initialiseBootTimePaging - 0x80000000
  blx r0
  ldr r0, =kernel_page_directory_start - 0x80000000
  mcr p15, 0, r0, c2, c0, 0
  mov r0, #0
  mcr p15, 0, r0, c8, c7, 0
  mov r0, #0x3
  mcr p15, 0, r0, c3, c0, 0
  mrc p15, 0, r0, c1, c0, 0
  orr r0, r0, #0x1
  mcr p15, 0, r0, c1, c0, 0
  ldr r0, =PagingMode
  bx r0
3:
  b 3 @ Halt

PagingMode:
.globl PagingMode
  ldr sp, =stack+0x4000 @ Set up the stack
  bl removeBootTimeIdentMapping
  bl startup
4:
  b 4 @ Halt

arch_TestAndSet:
.globl arch_TestAndSet
  swp r0, r0, [r1]
  bx lr

arch_yield:
.globl arch_yield
  swi #4
  bx lr

halt:
.globl halt
  mcr     p15, 0, r0, c7, c0, 4           @ Wait for interrupt
  mov     pc, lr

__aeabi_atexit:
    .globl __aeabi_atexit
raise:
    .globl raise
1:
    b 1b @ Halt
