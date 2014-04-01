/**
 * @file ArchInterrupts.cpp
 *
 */

#include "ArchInterrupts.h"
#include "structures.h"
#include "ports.h"
#include "kprintf.h"
#include "InterruptUtils.h"
#include "SegmentUtils.h"
#include "ArchThreads.h"

// parts of this code are taken from http://wiki.osdev.org/ARM_Integrator-CP_IRQTimerAndPIC
extern char* irq_switch_stack;

#define KEXP_TOPSWI \
  uint32      lr; \
  asm("mov sp, %[ps]" : : [ps]"g" ((uint32)irq_switch_stack+4096)); \
  asm("push {lr}"); \
  asm("push {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12}"); \
  asm("mov %[ps], lr" : [ps]"=r" (lr));

#define KEXP_BOTSWI \
  asm("pop {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12}"); \
  asm("LDM sp!, {pc}^")

#define KEXP_TOP3 \
  uint32      lr; \
  asm("mov sp, %[ps]" : : [ps]"g" ((uint32)irq_switch_stack+4096)); \
  asm("sub lr, lr, #4"); \
  asm("push {lr}"); \
  asm("push {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12}"); \
  asm("mrs r0, spsr"); \
  asm("push {r0}"); \
  asm("mov %[ps], lr" : [ps]"=r" (lr));

#define KEXP_BOT3 \
  asm("pop {r0}"); \
  asm("msr spsr, r0"); \
  asm("pop {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12}"); \
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

void __attribute__((naked)) k_exphandler_irq_entry() { KEXP_TOP3; exceptionHandler(lr, ARM4_XRQ_IRQ); KEXP_BOT3; }
void __attribute__((naked)) k_exphandler_fiq_entry() { KEXP_TOP3;  exceptionHandler(lr, ARM4_XRQ_FIQ); KEXP_BOT3; }
void __attribute__((naked)) k_exphandler_reset_entry() { KEXP_TOP3; exceptionHandler(lr, ARM4_XRQ_RESET); KEXP_BOT3; }
void __attribute__((naked)) k_exphandler_undef_entry() { KEXP_TOP3; exceptionHandler(lr, ARM4_XRQ_UNDEF); KEXP_BOT3; }
void __attribute__((naked)) k_exphandler_abrtp_entry() { KEXP_TOP3; exceptionHandler(lr, ARM4_XRQ_ABRTP); KEXP_BOT3; }
void __attribute__((naked)) k_exphandler_abrtd_entry() { KEXP_TOP3; exceptionHandler(lr, ARM4_XRQ_ABRTD); KEXP_BOT3; }
void __attribute__((naked)) k_exphandler_swi_entry() { KEXP_TOPSWI;   exceptionHandler(lr, ARM4_XRQ_SWINT); KEXP_BOT3; }

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
  uint32    *picmmio;

  /* initialize timer and PIC

    The timer interrupt line connects to the PIC. You can make
    the timer interrupt an IRQ or FIQ just by enabling the bit
    needed in either the IRQ or FIQ registers. Here I use the
    IRQ register. If you enable both IRQ and FIQ then FIQ will
    take priority and be used.
  */
  picmmio = (uint32*)0x14000000;
  picmmio[PIC_IRQ_ENABLESET] = (1<<5) | (1<<6) | (1<<7);

  uint32* t0mmio = (uint32*)0x13000000;
  t0mmio[REG_LOAD] = 0xffffff;
  t0mmio[REG_BGLOAD] = 0xffffff;
  t0mmio[REG_CTRL] = CTRL_ENABLE | CTRL_MODE_PERIODIC | CTRL_SIZE_32 | CTRL_DIV_NONE | CTRL_INT_ENABLE;
  t0mmio[REG_INTCLR] = ~0;    /* make sure interrupt is clear (might not be mandatory) */

}

void ArchInterrupts::disableTimer()
{
  uint32* t0mmio = (uint32*)0x13000000;
  t0mmio[REG_LOAD] = 0xffffff;
  t0mmio[REG_BGLOAD] = 0xffffff;
  t0mmio[REG_CTRL] = 0;
  t0mmio[REG_INTCLR] = ~0;    /* make sure interrupt is clear (might not be mandatory) */

}

void ArchInterrupts::enableKBD()
{
//  enableIRQ(1);
//  enableIRQ(9);
}

void ArchInterrupts::enableBDS()
{
//  enableIRQ(2);
//  enableIRQ(9);
//  enableIRQ(11);
//  enableIRQ(14);
//  enableIRQ(15);
}

void ArchInterrupts::disableKBD()
{
//  disableIRQ(1);
}

void ArchInterrupts::EndOfInterrupt(uint16 number) 
{
//  sendEOI(number);
}
extern "C" void arch_enableInterrupts();
void ArchInterrupts::enableInterrupts()
{
  arm4_xrqinstall(ARM4_XRQ_RESET, (void*)&k_exphandler_reset_entry);
  arm4_xrqinstall(ARM4_XRQ_UNDEF, (void*)&k_exphandler_undef_entry);
  arm4_xrqinstall(ARM4_XRQ_SWINT, (void*)&k_exphandler_swi_entry);
  arm4_xrqinstall(ARM4_XRQ_ABRTP, (void*)&k_exphandler_abrtp_entry);
  arm4_xrqinstall(ARM4_XRQ_ABRTD, (void*)&k_exphandler_abrtd_entry);
  arm4_xrqinstall(ARM4_XRQ_IRQ, (void*)&k_exphandler_irq_entry);
  arm4_xrqinstall(ARM4_XRQ_FIQ, (void*)&k_exphandler_fiq_entry);

  arm4_cpsrset(arm4_cpsrget() & ~(1 << 7));
  enableTimer();
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
