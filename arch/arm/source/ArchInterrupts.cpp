/**
 * @file ArchInterrupts.cpp
 *
 */

#include "ArchInterrupts.h"
#include "ports.h"
#include "kprintf.h"
#include "InterruptUtils.h"
#include "ArchThreads.h"
#include "atkbd.h"

extern struct KMI* kmi;

// parts of this code are taken from http://wiki.osdev.org/ARM_Integrator-CP_IRQTimerAndPIC

#define KEXP_TOP3 \
  asm("sub lr, lr, #4"); \
  KEXP_TOPSWI

#define KEXP_TOPSWI \
  asm("mov %[v], lr" : [v]"=r" (currentThreadInfo->pc));\
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
  asm("mrs r0, spsr"); \
  asm("mov %[v], r0" : [v]"=r" (currentThreadInfo->cpsr));\
  asm("mrs r0, cpsr \n\
       bic r0, r0, #0x1f \n\
       orr r0, r0, #0x1f \n\
       msr cpsr, r0 \n\
       mov r4, sp \n\
       mov %[lr], lr \n\
       bic r0, r0, #0x1f \n\
       orr r0, r0, #0x12 \n\
       msr cpsr, r0 \n\
       mov sp, r4 \n\
       mov %[sp], r4 \n\
       " : [sp]"=r" (currentThreadInfo->sp), [lr]"=r" (currentThreadInfo->lr));

#define KEXP_BOTSWI \
    KEXP_BOT3

#define KEXP_BOT3 \
  asm("mov r0, %[v]" : : [v]"r" (currentThreadInfo->cpsr));\
  asm("msr spsr, r0"); \
  asm("mov r0, %[v]" : : [v]"r" (currentThreadInfo->r0));\
  asm("mov r1, %[v]" : : [v]"r" (currentThreadInfo->r1));\
  asm("mov r2, %[v]" : : [v]"r" (currentThreadInfo->r2));\
  asm("mov r3, %[v]" : : [v]"r" (currentThreadInfo->r3));\
  asm("mov r4, %[v]" : : [v]"r" (currentThreadInfo->r4));\
  asm("mov r5, %[v]" : : [v]"r" (currentThreadInfo->r5));\
  asm("mov r6, %[v]" : : [v]"r" (currentThreadInfo->r6));\
  asm("mov r7, %[v]" : : [v]"r" (currentThreadInfo->r7));\
  asm("mov r8, %[v]" : : [v]"r" (currentThreadInfo->r8));\
  asm("mov r9, %[v]" : : [v]"r" (currentThreadInfo->r9));\
  asm("mov r10, %[v]" : : [v]"r" (currentThreadInfo->r10));\
  asm("mov r11, %[v]" : : [v]"r" (currentThreadInfo->r11));\
  asm("mov r12, %[v]" : : [v]"r" (currentThreadInfo->r12));\
  asm("mov sp, %[v]" : : [v]"r" (currentThreadInfo->sp));\
  asm("mrs r0, cpsr \n\
      bic r0, r0, #0x1f \n\
      orr r0, r0, #0x1f \n\
      msr cpsr, r0 \n\
      mov sp, %[sp] \n\
      mov lr, %[lr] \n\
      bic r0, r0, #0x1f \n\
      orr r0, r0, #0x12 \n\
      msr cpsr, r0 \n\
      " : : [sp]"r" (currentThreadInfo->sp), [lr]"r" (currentThreadInfo->lr));\
  asm("mov lr, %[v]" : : [v]"r" (currentThreadInfo->pc));\
  asm("push {lr}"); \
  asm("LDM sp!, {pc}^")

uint32 arm4_cpsrget()
{
    uint32      r;

    asm("mrs %[ps], cpsr" : [ps]"=r" (r));
    return r;
}

void arm4_cpsrset(uint32 r)
{
    asm("msr cpsr, %[ps]" : : [ps]"r" (r));
}
extern uint32 stack;

void __attribute__((naked)) k_exphandler_irq_entry() { KEXP_TOP3;  exceptionHandler(ARM4_XRQ_IRQ); KEXP_BOT3; }
void __attribute__((naked)) k_exphandler_fiq_entry() { KEXP_TOP3;  exceptionHandler(ARM4_XRQ_FIQ); KEXP_BOT3; }
void __attribute__((naked)) k_exphandler_reset_entry() { KEXP_TOP3; exceptionHandler(ARM4_XRQ_RESET); KEXP_BOT3; }
void __attribute__((naked)) k_exphandler_undef_entry() { KEXP_TOP3; exceptionHandler(ARM4_XRQ_UNDEF); KEXP_BOT3; }
void __attribute__((naked)) k_exphandler_abrtp_entry() { KEXP_TOP3; exceptionHandler(ARM4_XRQ_ABRTP); KEXP_BOT3; }
void __attribute__((naked)) k_exphandler_abrtd_entry() { KEXP_TOP3; exceptionHandler(ARM4_XRQ_ABRTD); KEXP_BOT3; }
void __attribute__((naked)) k_exphandler_swi_entry() { KEXP_TOPSWI; exceptionHandler(ARM4_XRQ_SWINT); KEXP_BOTSWI; }

void arm4_xrqinstall(uint32 ndx, void *addr) {
  char buf[32];
    uint32      *v;

    v = (uint32*)0x0;
  v[ndx] = 0xEA000000 | (((uint32)addr - (8 + (4 * ndx))) >> 2);
}

extern "C" void initialisePL190();
extern "C" void arch_enableIRQ(uint32);
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
  uint32* picmmio = (uint32*)0x14000000;
  picmmio[PIC_IRQ_ENABLESET] = (1<<5);

  uint32* t0mmio = (uint32*)0x13000000;
  t0mmio[REG_LOAD] = 0x2ffffff;
  t0mmio[REG_BGLOAD] = 0x2ffffff;
  t0mmio[REG_CTRL] = CTRL_ENABLE | CTRL_MODE_PERIODIC | CTRL_DIV_NONE | CTRL_SIZE_32 | CTRL_INT_ENABLE;
  t0mmio[REG_INTCLR] = ~0;    /* make sure interrupt is clear (might not be mandatory) */

}

void ArchInterrupts::disableTimer()
{
  uint32* t0mmio = (uint32*)0x13000000;
  t0mmio[REG_CTRL] = 0;

}

void ArchInterrupts::enableKBD()
{
  uint32* picmmio = (uint32*)0x14000000;
  picmmio[PIC_IRQ_ENABLESET] = (1<<3);

  kmi = (struct KMI*)0x18000000;
  kmi->cr = 0x14;
  kmi->data = 0xF4;
  while(!kmi->stat & 0x10);
}

void ArchInterrupts::disableKBD()
{
  kmi->cr = 0x0;
}

void ArchInterrupts::enableMMC()
{
  uint32* picmmio = (uint32*)0x14000000;
  picmmio[PIC_IRQ_ENABLESET] = (1<<3);

  kmi = (struct KMI*)0x18000000;
  kmi->cr = 0x14;
  kmi->data = 0xF4;
  while(!kmi->stat & 0x10);
}

void ArchInterrupts::disableMMC()
{
  kmi->cr = 0x0;
}

extern "C" void arch_enableInterrupts();
void ArchInterrupts::enableInterrupts()
{
  arm4_cpsrset(arm4_cpsrget() & ~(1 << 7));
}
extern "C" void arch_disableInterrupts();
bool ArchInterrupts::disableInterrupts()
{
  arm4_cpsrset(arm4_cpsrget() | (1 << 7));
}

bool ArchInterrupts::testIFSet()
{
  return !(arm4_cpsrget() & (1 << 7));
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
