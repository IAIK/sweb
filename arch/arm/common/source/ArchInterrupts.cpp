/**
 * @file ArchInterrupts.cpp
 *
 */

#include "types.h"
#include "ArchInterrupts.h"
#include "kprintf.h"
#include "InterruptUtils.h"
#include "ArchThreads.h"
#include "ArchBoardSpecific.h"

extern uint8 boot_stack[];

#define KEXP_TOP3 \
  asm("sub lr, lr, #4"); \
  KEXP_TOPSWI

#define KEXP_TOPSWI \
  asm("push {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12}");\
  asm("add sp, sp, #0x34");\
  asm("mov %[v], lr" : [v]"=r" (currentThreadInfo->pc));\
  asm("mrs r0, spsr"); \
  asm("mov %[v], r0" : [v]"=r" (currentThreadInfo->cpsr));\
  asm("mrs r0, cpsr \n\
       bic r0, r0, #0xdf \n\
       orr r0, r0, #0xdf \n\
       msr cpsr, r0 \n\
      ");\
  asm("mov %[v], sp" : [v]"=r" (currentThreadInfo->sp));\
  asm("mov %[v], lr" : [v]"=r" (currentThreadInfo->lr));\
  if (currentThreadInfo->sp < 0x80000000) { asm("mov sp, %[v]" : : [v]"r" (currentThreadInfo->sp0)); }\
  storeRegisters();

void storeRegisters()
{
  uint32* stack = ((uint32*)boot_stack) + 0x1000;
  currentThreadInfo->r12 = stack[-1];
  currentThreadInfo->r11 = stack[-2];
  currentThreadInfo->r10 = stack[-3];
  currentThreadInfo->r9 = stack[-4];
  currentThreadInfo->r8 = stack[-5];
  currentThreadInfo->r7 = stack[-6];
  currentThreadInfo->r6 = stack[-7];
  currentThreadInfo->r5 = stack[-8];
  currentThreadInfo->r4 = stack[-9];
  currentThreadInfo->r3 = stack[-10];
  currentThreadInfo->r2 = stack[-11];
  currentThreadInfo->r1 = stack[-12];
  currentThreadInfo->r0 = stack[-13];
}

#define KEXP_BOT3 \
  asm("mov lr, %[v]" : : [v]"r" (currentThreadInfo->lr));\
  asm("mov sp, %[v]" : : [v]"r" (currentThreadInfo->sp));\
  asm("mrs r0, cpsr \n\
       bic r0, r0, #0xdf \n\
       orr r0, r0, #0xd3 \n\
       msr cpsr, r0 \n\
      ");\
  asm("mov r0, %[v]" : : [v]"r" (currentThreadInfo->cpsr));\
  asm("mov lr, %[v]" : : [v]"r" (currentThreadInfo->pc));\
  asm("msr spsr, r0"); \
  asm("mov r3, %[v]" : : [v]"r" (currentThreadInfo->r12));\
  asm("push {r3}");\
  asm("mov r3, %[v]" : : [v]"r" (currentThreadInfo->r11));\
  asm("push {r3}");\
  asm("mov r3, %[v]" : : [v]"r" (currentThreadInfo->r10));\
  asm("push {r3}");\
  asm("mov r3, %[v]" : : [v]"r" (currentThreadInfo->r9));\
  asm("push {r3}");\
  asm("mov r3, %[v]" : : [v]"r" (currentThreadInfo->r8));\
  asm("push {r3}");\
  asm("mov r3, %[v]" : : [v]"r" (currentThreadInfo->r7));\
  asm("push {r3}");\
  asm("mov r3, %[v]" : : [v]"r" (currentThreadInfo->r6));\
  asm("push {r3}");\
  asm("mov r3, %[v]" : : [v]"r" (currentThreadInfo->r5));\
  asm("push {r3}");\
  asm("mov r3, %[v]" : : [v]"r" (currentThreadInfo->r4));\
  asm("push {r3}");\
  asm("mov r3, %[v]" : : [v]"r" (currentThreadInfo->r3));\
  asm("push {r3}");\
  asm("mov r3, %[v]" : : [v]"r" (currentThreadInfo->r2));\
  asm("push {r3}");\
  asm("mov r3, %[v]" : : [v]"r" (currentThreadInfo->r1));\
  asm("push {r3}");\
  asm("mov r3, %[v]" : : [v]"r" (currentThreadInfo->r0));\
  asm("push {r3}");\
  asm("pop {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12}");\
  asm("movs pc, lr")

uint32 arm4_cpsrget()
{
  uint32 r;

  asm("mrs %[ps], cpsr" : [ps]"=r" (r));
  return r;
}

void arm4_cpsrset(uint32 r)
{
  asm("msr cpsr, %[ps]" : : [ps]"r" (r));
}

void __naked__ k_exphandler_irq_entry() { KEXP_TOP3;  void (*eh)(uint32 type) = &exceptionHandler; eh(ARM4_XRQ_IRQ); KEXP_BOT3; }
void __naked__ k_exphandler_fiq_entry() { KEXP_TOP3; void (*eh)(uint32 type) = &exceptionHandler; eh(ARM4_XRQ_FIQ); KEXP_BOT3; }
void __naked__ k_exphandler_reset_entry() { KEXP_TOP3; void (*eh)(uint32 type) = &exceptionHandler; eh(ARM4_XRQ_RESET); KEXP_BOT3; }
void __naked__ k_exphandler_undef_entry() { KEXP_TOP3; void (*eh)(uint32 type) = &exceptionHandler; eh(ARM4_XRQ_UNDEF); KEXP_BOT3; }
void __naked__ k_exphandler_abrtp_entry() { KEXP_TOP3; void (*eh)(uint32 type) = &exceptionHandler; eh(ARM4_XRQ_ABRTP); KEXP_BOT3; }
void __naked__ k_exphandler_abrtd_entry() { KEXP_TOP3; currentThreadInfo->pc -= 4; void (*eh)(uint32 type) = &exceptionHandler; eh(ARM4_XRQ_ABRTD); KEXP_BOT3; }
void __naked__ k_exphandler_swi_entry() { KEXP_TOPSWI; void (*eh)(uint32 type) = &exceptionHandler; eh(ARM4_XRQ_SWINT); KEXP_BOT3; }

void arm4_xrqinstall(uint32 ndx, void *addr, uint32 mode)
{
  ((uint32*) 0x0)[ndx] = 0xEA000000 | (((uint32) addr - (8 + (4 * ndx))) >> 2);
  asm("mrs r0, cpsr \n\
         bic r0, r0, #0xdf \n\
         orr r0, r0, %[v] \n\
         msr cpsr, r0" : : [v]"r" (mode));
  uint32* stack = ((uint32*)boot_stack) + 0x1000;
  asm("mov sp, %[v]" : : [v]"r" (stack));
  asm("mrs r0, cpsr \n\
       bic r0, r0, #0xdf \n\
       orr r0, r0, #0xdf \n\
       msr cpsr, r0");
}

void ArchInterrupts::initialise()
{
  arm4_xrqinstall(ARM4_XRQ_RESET, (void*)&k_exphandler_reset_entry, 0xD3);
  arm4_xrqinstall(ARM4_XRQ_UNDEF, (void*)&k_exphandler_undef_entry, 0xDB);
  arm4_xrqinstall(ARM4_XRQ_SWINT, (void*)&k_exphandler_swi_entry, 0xD3);
  arm4_xrqinstall(ARM4_XRQ_ABRTP, (void*)&k_exphandler_abrtp_entry, 0xD7);
  arm4_xrqinstall(ARM4_XRQ_ABRTD, (void*)&k_exphandler_abrtd_entry, 0xD7);
  arm4_xrqinstall(ARM4_XRQ_IRQ, (void*)&k_exphandler_irq_entry, 0xD2);
  arm4_xrqinstall(ARM4_XRQ_FIQ, (void*)&k_exphandler_fiq_entry, 0xD1);


  InterruptUtils::initialise();
}

void ArchInterrupts::enableTimer()
{
  ArchBoardSpecific::enableTimer();
}

void ArchInterrupts::disableTimer()
{
  ArchBoardSpecific::disableTimer();
}

void ArchInterrupts::enableKBD()
{
  ArchBoardSpecific::enableKBD();
}

void ArchInterrupts::disableKBD()
{
  ArchBoardSpecific::disableKBD();
}

extern "C" void arch_enableInterrupts();
void ArchInterrupts::enableInterrupts()
{
  arm4_cpsrset(arm4_cpsrget() & ~((1 << 7) | (1 << 6)));
}
extern "C" void arch_disableInterrupts();
bool ArchInterrupts::disableInterrupts()
{
  arm4_cpsrset(arm4_cpsrget() | ((1 << 7) | (1 << 6)));
}

bool ArchInterrupts::testIFSet()
{
  return !(arm4_cpsrget() & ((1 << 7) | (1 << 6)));
}

void ArchInterrupts::yieldIfIFSet()
{
  extern uint32 boot_completed;
  if (boot_completed && currentThread && testIFSet())
  {
    ArchThreads::yield();
  }
  else
  {
    __asm__ __volatile__("nop");
  }
}

extern "C" void switchTTBR0(uint32 ttbr0)
{
  asm("mcr p15, 0, %[v], c2, c0, 0" : : [v]"r" (ttbr0));
}


extern "C" void memory_barrier() // from https://github.com/raspberrypi/firmware/wiki/Accessing-mailboxes
{
  asm("mcr p15, 0, %[v], c8, c7, 0\n"  //      tlb flush
      "mcr p15, 0, %[v], c7, c6, 0\n"  //      Invalidate Entire Data Cache
      "mcr p15, 0, %[v], c7, c10, 0\n" //      Clean Entire Data Cache
      "mcr p15, 0, %[v], c7, c14, 0\n" //      Clean and Invalidate Entire Data Cache
      "mcr p15, 0, %[v], c7, c10, 4\n" //      Data Synchronization Barrier
      "mcr p15, 0, %[v], c7, c10, 5\n" //      Data Memory Barrier
      :
      : [v]"r"(0));
}
