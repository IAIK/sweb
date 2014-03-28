.text

.comm kernel_page_directory_start, 0x4000 @ Reserve 16k space for page directory

.bss
.comm stack, 0x10000 @ Reserve 64k stack in the BSS

.text

interrupt_vector_table:
    b . @ Reset
    b .
    b . @ SWI instruction
    b .
    b .
    b .
    b .
    b .

entry:
.globl entry
  ldr sp, =stack - 0x80000000+0x10000 @ Set up the stack
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
  ldr sp, =stack - 0x80000000+0x10000 @ Set up the stack
  ldr r0, =PagingMode
  bx r0
3:
  b 3 @ Halt

PagingMode:
.globl PagingMode
  bl removeBootTimeIdentMapping
  bl startup
4:
  b 4 @ Halt

__aeabi_atexit:
    .globl __aeabi_atexit
raise:
    .globl raise
1:
    b 1b @ Halt
