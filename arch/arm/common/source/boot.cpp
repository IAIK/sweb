#include "types.h"
#include "board_constants.h"
#include "init_boottime_pagetables.h"
#include "assert.h"

extern "C" void startup();
extern "C" void __naked__ PagingMode();

uint8 interrupt_stack[0x4000] __attribute__((aligned(0x4000)));
page_directory_entry kernel_page_directory[0x1000] __attribute__((aligned(0x4000))); // space for page directory

#define BOOT_OFFSET (BOARD_LOAD_BASE - 0x80000000)

extern "C" void __naked__ entry()
{
  asm("mov fp, #0\n"
      "mov sp, %[v]" : : [v]"r"(interrupt_stack + BOOT_OFFSET + 0x4000)); // Set up the stack
  void (*initialiseBootTimePagingPTR)() = (void(*)())((uint8*)&initialiseBootTimePaging + BOOT_OFFSET);
  initialiseBootTimePagingPTR();
  asm("mcr p15, 0, %[v], c2, c0, 0\n" : : [v]"r"(((uint8*)kernel_page_directory) + BOOT_OFFSET)); // set ttbr0
  asm("mcr p15, 0, %[v], c8, c7, 0\n" : : [v]"r"(0)); // tlb flush
  asm("mcr p15, 0, %[v], c3, c0, 0\n" : : [v]"r"(3)); // set domain access control (full address space access for userspace)
  asm("mrc p15, 0, r0, c1, c0, 0\n"
      "orr r0, r0, #0x1\n"          // set paging bit
      "bic r0, r0, #0x2\n"          // disable alignment fault bit
      "orr r0, r0, #0x400000\n"     // set unaligned memory access bit
      "mcr p15, 0, r0, c1, c0, 0\n" // enable paging
     );
  void (*PagingModePTR)() = &PagingMode; // create a blx jump instead of a bl jump
  PagingModePTR();
  assert(false && "it should be impossible to get to this point");
}

extern "C" void __naked__ PagingMode()
{
  asm("mrs r0, cpsr\n"
      "bic r0, r0, #0xdf\n"
      "orr r0, r0, #0xdf\n"
      "msr cpsr, r0\n");
  asm("mov sp, %[v]\n" : : [v]"r"(interrupt_stack+4096)); // Set up the stack
  removeBootTimeIdentMapping();
  void (*startupPTR)() = &startup; // create a blx jump instead of a bl jump
  startupPTR();
  assert(false && "you should never get to this point");
}
