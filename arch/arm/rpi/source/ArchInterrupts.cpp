/**
 * @file ArchInterrupts.cpp
 *
 */

#include "ArchInterrupts.h"
#include "kprintf.h"
#include "InterruptUtils.h"
#include "ArchThreads.h"
#include "atkbd.h"

#define KEXP_TOP3 \
  asm("sub lr, lr, #4"); \
  KEXP_TOPSWI

#define KEXP_TOPSWI \
  asm("mov %[v], r0" : [v]"=r" (currentThreadInfo->r0));\
  asm("mov %[v], r1" : [v]"=r" (currentThreadInfo->r1));\
  asm("mov %[v], r2" : [v]"=r" (currentThreadInfo->r2));\
  asm("mov %[v], r3" : [v]"=r" (currentThreadInfo->r3));\
  asm("mov %[v], r4" : [v]"=r" (currentThreadInfo->r4));\
  asm("mov %[v], r5" : [v]"=r" (currentThreadInfo->r5));\
  asm("mov %[v], r6" : [v]"=r" (currentThreadInfo->r6));\
  asm("mov %[v], r7" : [v]"=r" (currentThreadInfo->r7));\
  asm("mov %[v], r8" : [v]"=r" (currentThreadInfo->r8));\
  asm("mov %[v], r9" : [v]"=r" (currentThreadInfo->r9));\
  asm("mov %[v], r10" : [v]"=r" (currentThreadInfo->r10));\
  asm("mov %[v], r11" : [v]"=r" (currentThreadInfo->r11));\
  asm("mov %[v], r12" : [v]"=r" (currentThreadInfo->r12));\
  asm("mov %[v], lr" : [v]"=r" (currentThreadInfo->pc));\
  asm("mrs r12, spsr"); \
  asm("mov %[v], r12" : [v]"=r" (currentThreadInfo->cpsr));\
  asm("mrs r12, cpsr \n\
       bic r12, r12, #0xdf \n\
       orr r12, r12, #0xdf \n\
       msr cpsr, r12 \n\
      ");\
  asm("mov %[v], sp" : [v]"=r" (currentThreadInfo->sp));\
  asm("mov %[v], lr" : [v]"=r" (currentThreadInfo->lr));\
  if (currentThreadInfo->sp < 0x80000000) { asm("mov sp, %[v]" : : [v]"r" (currentThreadInfo->sp0)); }\
  asm("mov r0, %[v]" : : [v]"r" (currentThreadInfo->r0));


#define KEXP_BOT3 \
  asm("mov lr, %[v]" : : [v]"r" (currentThreadInfo->lr));\
  asm("mov sp, %[v]" : : [v]"r" (currentThreadInfo->sp));\
  asm("mrs r12, cpsr \n\
       bic r12, r12, #0xdf \n\
       orr r12, r12, #0xd3 \n\
       msr cpsr, r12 \n\
      ");\
  asm("mov r12, %[v]" : : [v]"r" (currentThreadInfo->cpsr));\
  asm("msr spsr, r12"); \
  asm("mov lr, %[v]" : : [v]"r" (currentThreadInfo->pc));\
  asm("mov r12, %[v]" : : [v]"r" (currentThreadInfo->r12));\
  asm("mov r11, %[v]" : : [v]"r" (currentThreadInfo->r11));\
  asm("mov r10, %[v]" : : [v]"r" (currentThreadInfo->r10));\
  asm("mov r9, %[v]" : : [v]"r" (currentThreadInfo->r9));\
  asm("mov r8, %[v]" : : [v]"r" (currentThreadInfo->r8));\
  asm("mov r7, %[v]" : : [v]"r" (currentThreadInfo->r7));\
  asm("mov r6, %[v]" : : [v]"r" (currentThreadInfo->r6));\
  asm("mov r5, %[v]" : : [v]"r" (currentThreadInfo->r5));\
  asm("mov r4, %[v]" : : [v]"r" (currentThreadInfo->r4));\
  asm("mov r3, %[v]" : : [v]"r" (currentThreadInfo->r3));\
  asm("mov r2, %[v]" : : [v]"r" (currentThreadInfo->r2));\
  asm("mov r1, %[v]" : : [v]"r" (currentThreadInfo->r1));\
  asm("mov r0, %[v]" : : [v]"r" (currentThreadInfo->r0));\
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

void __attribute__((naked)) k_exphandler_irq_entry() { KEXP_TOP3; void (*eh)(uint32 type) = &exceptionHandler; eh(ARM4_XRQ_IRQ); KEXP_BOT3; }
void __attribute__((naked)) k_exphandler_fiq_entry() { KEXP_TOP3; void (*eh)(uint32 type) = &exceptionHandler; eh(ARM4_XRQ_FIQ); KEXP_BOT3; }
void __attribute__((naked)) k_exphandler_reset_entry() { KEXP_TOP3; void (*eh)(uint32 type) = &exceptionHandler; eh(ARM4_XRQ_RESET); KEXP_BOT3; }
void __attribute__((naked)) k_exphandler_undef_entry() { KEXP_TOP3; void (*eh)(uint32 type) = &exceptionHandler; eh(ARM4_XRQ_UNDEF); KEXP_BOT3; }
void __attribute__((naked)) k_exphandler_abrtp_entry() { KEXP_TOP3; void (*eh)(uint32 type) = &exceptionHandler; eh(ARM4_XRQ_ABRTP); KEXP_BOT3; }
void __attribute__((naked)) k_exphandler_abrtd_entry() { KEXP_TOP3; currentThreadInfo->pc -= 4; void (*eh)(uint32 type) = &exceptionHandler; eh(ARM4_XRQ_ABRTD); KEXP_BOT3; }
void __attribute__((naked)) k_exphandler_swi_entry() { KEXP_TOPSWI; void (*eh)(uint32 type) = &exceptionHandler; eh(ARM4_XRQ_SWINT); KEXP_BOT3; }

void arm4_xrqinstall(uint32 ndx, void *addr)
{
  ((uint32*) 0x0)[ndx] = 0xEA000000 | (((uint32) addr - (8 + (4 * ndx))) >> 2);
}

void ArchInterrupts::initialise()
{
  arm4_xrqinstall(ARM4_XRQ_RESET, (void*)&k_exphandler_reset_entry);
  arm4_xrqinstall(ARM4_XRQ_UNDEF, (void*)&k_exphandler_undef_entry);
  arm4_xrqinstall(ARM4_XRQ_SWINT, (void*)&k_exphandler_swi_entry);
  arm4_xrqinstall(ARM4_XRQ_ABRTP, (void*)&k_exphandler_abrtp_entry);
  arm4_xrqinstall(ARM4_XRQ_ABRTD, (void*)&k_exphandler_abrtd_entry);
  arm4_xrqinstall(ARM4_XRQ_IRQ, (void*)&k_exphandler_irq_entry);
  arm4_xrqinstall(ARM4_XRQ_FIQ, (void*)&k_exphandler_fiq_entry);

  InterruptUtils::initialise();
}

void ArchInterrupts::enableTimer()
{
  uint32* pic_base_enable = (uint32*)0x9000B218;
  *pic_base_enable = 0x1;

  uint32* timer_load = (uint32*)0x9000B400;
  uint32* timer_value = timer_load + 1;
  *timer_load = 0x1000;
  uint32* timer_control = timer_load + 2;
  *timer_control = (1 << 7) | (1 << 5) | (1 << 2);
  uint32* timer_clear = timer_load + 3;
  *timer_clear = 0x1;


}

void ArchInterrupts::disableTimer()
{
  uint32* timer_load = (uint32*)0x9000B400;
  uint32* timer_control = (uint32*)0x9000B40C;
}

void ArchInterrupts::enableKBD()
{

}

void ArchInterrupts::disableKBD()
{
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
