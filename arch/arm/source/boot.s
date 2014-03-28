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
  bl removeBootTimeIdentMapping
3:
  b 3 @ Halt



LS_Code:
    .globl LS_Code
LS_Data:
    .globl LS_Data
arch_pageFaultHandler:
    .globl arch_pageFaultHandler
arch_TestAndSet:
    .globl arch_TestAndSet
kernel_page_tables_start:
    .globl kernel_page_tables_start
ro_data_end_address:
    .globl ro_data_end_address
multi_boot_structure_pointer:
    .globl multi_boot_structure_pointer
stab_start_address_nr:
    .globl stab_start_address_nr
stabstr_start_address_nr:
    .globl stabstr_start_address_nr
stabstr_end_address_nr:
    .globl stabstr_end_address_nr
stab_end_address_nr:
    .globl stab_end_address_nr
tss_selector:
    .globl tss_selector
kernel_end_address:
    .globl kernel_end_address
reload_segements:
    .globl reload_segements
arch_dummyHandler:
    .globl arch_dummyHandler
arch_syscallHandler:
    .globl arch_syscallHandler
arch_irqHandler:
    .globl arch_irqHandler
arch_errorHandler:
    .globl arch_errorHandler
arch_switchThreadToUserPageDirChange:
    .globl arch_switchThreadToUserPageDirChange
arch_switchThreadKernelToKernelPageDirChange:
    .globl arch_switchThreadKernelToKernelPageDirChange
__aeabi_atexit:
    .globl __aeabi_atexit
__aeabi_idivmod:
    .globl __aeabi_idivmod
__aeabi_idiv:
    .globl __aeabi_idiv
__aeabi_uidivmod:
    .globl __aeabi_uidivmod
__aeabi_uidiv:
    .globl __aeabi_uidiv
1:
    b 1b @ Halt
