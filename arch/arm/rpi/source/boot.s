.bss
.comm stack, 0x4000 @ Reserve 16k stack in the BSS

.text

.comm kernel_page_directory_start, 0x4000 @ Reserve 16k space for page directory

entry:
.globl entry
  ldr sp, =stack - 0x80000000+0x4000 @ Set up the stack
  mov fp, #0
  ldr r0, =initialiseBootTimePaging - 0x80000000
  blx r0
  ldr r0, =kernel_page_directory_start - 0x80000000
  mcr p15, 0, r0, c2, c0, 0 @ set ttbr0
  mov r0, #0
  mcr p15, 0, r0, c8, c7, 0 @ tlb flush
  mov r0, #0x3
  mcr p15, 0, r0, c3, c0, 0 @ set domain access control (full address space access for userspace)
  mrc p15, 0, r0, c1, c0, 0
  orr r0, r0, #0x1          @ set paging bit
  orr r0, r0, #0x400000     @ set unaligned memory access bit
  bic r0, r0, #0x2          @ disable alignment fault bit
  mcr p15, 0, r0, c1, c0, 0 @ enable paging
  ldr r0, =PagingMode
  bx r0
3:
  b 3 @ Halt

PagingMode:
.globl PagingMode
  mrs r0, cpsr
  bic r0, r0, #0x1f
  orr r0, r0, #0x1f
  msr cpsr, r0
  ldr sp, =stack+0x4000 @ Set up the stack
  bl removeBootTimeIdentMapping
  ldr r0, =startup
  bx r0
  bl startup
4:
  b 4 @ Halt

arch_TestAndSet:
.globl arch_TestAndSet
  swp r0, r2, [r3]
  @ this would be better according to arm manual, but it does not work:
  @ ldrex r0, [r3]
  @ strex r1, r2, [r3]
  @ cmp r1, #0
  @ bne arch_TestAndSet
  bx lr

switchTTBR0:
.globl switchTTBR0
  mcr p15, 0, r0, c2, c0, 0
  bx lr

arch_yield:
.stabs "arch_yield",36,0,0,arch_yield
.globl arch_yield
  swi #0xffff
  bx lr

halt:
.globl halt
  mcr     p15, 0, r0, c7, c0, 4           @ Wait for interrupt
  bx lr

memory_barrier: @ from https://github.com/raspberrypi/firmware/wiki/Accessing-mailboxes
.globl memory_barrier
   mov r0, #0
   mcr p15, 0, r0, c8, c7, 0       @ tlb flush
   mcr p15, 0, r0, C7, C6, 0       @ Invalidate Entire Data Cache
   mcr p15, 0, r0, c7, c10, 0      @ Clean Entire Data Cache
   mcr p15, 0, r0, c7, c14, 0      @ Clean and Invalidate Entire Data Cache
   mcr p15, 0, r0, c7, c10, 4      @ Data Synchronization Barrier
   mcr p15, 0, r0, c7, c10, 5      @ Data Memory Barrier
   bx lr

__aeabi_atexit:
    .globl __aeabi_atexit
raise:
    .globl raise
1:
    b 1b @ Halt
