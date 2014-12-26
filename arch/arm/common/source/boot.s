.bss
.comm interrupt_stack, 0x4000 @ Reserve 16k stack in the BSS

.text

.comm kernel_page_directory_start, 0x4000 @ Reserve 16k space for page directory

entry:
.globl entry
  ldr sp, =interrupt_stack + (BOARD_LOAD_BASE - 0x80000000) + 0x4000 @ Set up the stack
  mov fp, #0
  ldr r0, =initialiseBootTimePaging + (BOARD_LOAD_BASE - 0x80000000)
  blx r0
  ldr r0, =kernel_page_directory_start + (BOARD_LOAD_BASE - 0x80000000)
  mcr p15, 0, r0, c2, c0, 0 @ set ttbr0
  mov r0, #0
  mcr p15, 0, r0, c8, c7, 0 @ tlb flush
  mov r0, #0x3
  mcr p15, 0, r0, c3, c0, 0 @ set domain access control (full address space access for userspace)
  mrc p15, 0, r0, c1, c0, 0
  orr r0, r0, #0x1          @ set paging bit
  bic r0, r0, #0x2          @ disable alignment fault bit
  orr r0, r0, #0x400000     @ set unaligned memory access bit
  mcr p15, 0, r0, c1, c0, 0 @ enable paging
  ldr r0, =PagingMode
  bx r0
  b . @ Halt

PagingMode:
.globl PagingMode
  mrs r0, cpsr
  bic r0, r0, #0xdf
  orr r0, r0, #0xdf
  msr cpsr, r0
  ldr sp, =interrupt_stack+0x4000 @ Set up the stack
  bl removeBootTimeIdentMapping
  ldr r0, =startup
  bx r0
  bl startup
  b . @ Halt
