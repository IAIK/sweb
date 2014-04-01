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

bool ArchInterrupts::enabled = false;
extern char* kstackexc;

#define ARM4_XRQ_RESET   0x00
#define ARM4_XRQ_UNDEF   0x01
#define ARM4_XRQ_SWINT   0x02
#define ARM4_XRQ_ABRTP   0x03
#define ARM4_XRQ_ABRTD   0x04
#define ARM4_XRQ_RESV1   0x05
#define ARM4_XRQ_IRQ     0x06
#define ARM4_XRQ_FIQ     0x07

#define REG_INTCLR    0x03

#define KEXP_TOPSWI \
  uint32      lr; \
  asm("mov sp, %[ps]" : : [ps]"g" ((uint32)kstackexc+4096)); \
  asm("push {lr}"); \
  asm("push {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12}"); \
  asm("mov %[ps], lr" : [ps]"=r" (lr));

#define KEXP_TOP3 \
  uint32      lr; \
  asm("mov sp, %[ps]" : : [ps]"g" ((uint32)kstackexc+4096)); \
  asm("sub lr, lr, #4"); \
  asm("push {lr}"); \
  asm("push {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12}"); \
  asm("mov %[ps], lr" : [ps]"=r" (lr));

#define KEXP_BOT3 \
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

void arm4_xrqenable_fiq()
{
    arm4_cpsrset(arm4_cpsrget() & ~(1 << 6));
}

void arm4_xrqenable_irq()
{
    arm4_cpsrset(arm4_cpsrget() & ~(1 << 7));
}

void k_exphandler(uint32 lr, uint32 type) {
  uint32      *t0mmio;
  uint32      swi;

  kprintfd("H");

  /*      clear interrupt in timer HW so it will lower its line to the PIC
                and therefore allow the PIC to lower its line to the CPU

    if you do not clear it, an interrupt will
    be immediantly raised apon return from this
    interrupt
  */
  t0mmio = (uint32*)0x13000000;

  t0mmio[REG_INTCLR] = 1;

  /*
    Get SWI argument (index).
  */
  if (type == ARM4_XRQ_SWINT) {
    swi = ((uint32*)((uint32)lr - 4))[0] & 0xffff;

    if (swi == 4) {
      kprintfd("@");
    }
  }

  if (type != ARM4_XRQ_IRQ && type != ARM4_XRQ_FIQ && type != ARM4_XRQ_SWINT) {
    /*
      Ensure, the exception return code is correctly handling LR with the
      correct offset. I am using the same return for everything except SWI,
      which requires that LR not be offset before return.
    */
    kprintfd("!");
    for(;;);
  }

  return;
}

void __attribute__((naked)) k_exphandler_irq_entry() { KEXP_TOP3; k_exphandler(lr, ARM4_XRQ_IRQ); KEXP_BOT3; }
void __attribute__((naked)) k_exphandler_fiq_entry() { KEXP_TOP3;  k_exphandler(lr, ARM4_XRQ_FIQ); KEXP_BOT3; }
void __attribute__((naked)) k_exphandler_reset_entry() { KEXP_TOP3; k_exphandler(lr, ARM4_XRQ_RESET); KEXP_BOT3; }
void __attribute__((naked)) k_exphandler_undef_entry() { KEXP_TOP3; k_exphandler(lr, ARM4_XRQ_UNDEF); KEXP_BOT3; }
void __attribute__((naked)) k_exphandler_abrtp_entry() { KEXP_TOP3; k_exphandler(lr, ARM4_XRQ_ABRTP); KEXP_BOT3; }
void __attribute__((naked)) k_exphandler_abrtd_entry() { KEXP_TOP3; k_exphandler(lr, ARM4_XRQ_ABRTD); KEXP_BOT3; }
void __attribute__((naked)) k_exphandler_swi_entry() { KEXP_TOPSWI;   k_exphandler(lr, ARM4_XRQ_SWINT); KEXP_BOT3; }

void arm4_xrqinstall(uint32 ndx, void *addr) {
  char buf[32];
    uint32      *v;

    v = (uint32*)0x0;
  v[ndx] = 0xEA000000 | (((uint32)addr - (8 + (4 * ndx))) >> 2);
}

#define PIC_IRQ_ENABLESET   0x2
#define REG_LOAD    0x00
#define REG_CTRL    0x02
#define REG_BGLOAD    0x06
#define CTRL_ENABLE     0x80
#define CTRL_MODE_PERIODIC  0x40
#define CTRL_INT_ENABLE   (1<<5)
#define CTRL_DIV_NONE   0x00
#define CTRL_SIZE_32    0x02

extern "C" void initialisePL190();
extern "C" void arch_enableIRQ(uint32);
void ArchInterrupts::initialise()
{
  uint32    *t0mmio;
  uint32    *picmmio;

  arm4_xrqinstall(ARM4_XRQ_RESET, (void*)&k_exphandler_reset_entry);
  arm4_xrqinstall(ARM4_XRQ_UNDEF, (void*)&k_exphandler_undef_entry);
  arm4_xrqinstall(ARM4_XRQ_SWINT, (void*)&k_exphandler_swi_entry);
  arm4_xrqinstall(ARM4_XRQ_ABRTP, (void*)&k_exphandler_abrtp_entry);
  arm4_xrqinstall(ARM4_XRQ_ABRTD, (void*)&k_exphandler_abrtd_entry);
  arm4_xrqinstall(ARM4_XRQ_IRQ, (void*)&k_exphandler_irq_entry);
  arm4_xrqinstall(ARM4_XRQ_FIQ, (void*)&k_exphandler_fiq_entry);

  asm("swi #4");

  /* enable IRQ */
  arm4_cpsrset(arm4_cpsrget() & ~(1 << 7));

  /* initialize timer and PIC

    The timer interrupt line connects to the PIC. You can make
    the timer interrupt an IRQ or FIQ just by enabling the bit
    needed in either the IRQ or FIQ registers. Here I use the
    IRQ register. If you enable both IRQ and FIQ then FIQ will
    take priority and be used.
  */
  picmmio = (uint32*)0x14000000;
  picmmio[PIC_IRQ_ENABLESET] = (1<<5) | (1<<6) | (1<<7);

  /*
    See datasheet for timer initialization details.
  */
  t0mmio = (uint32*)0x13000000;
  t0mmio[REG_LOAD] = 0xffffff;
  t0mmio[REG_BGLOAD] = 0xffffff;
  t0mmio[REG_CTRL] = CTRL_ENABLE | CTRL_MODE_PERIODIC | CTRL_SIZE_32 | CTRL_DIV_NONE | CTRL_INT_ENABLE;
  t0mmio[REG_INTCLR] = ~0;    /* make sure interrupt is clear (might not be mandatory) */
  for(;;);
  //initialisePL190();
  SegmentUtils::initialise();
  InterruptUtils::initialise();
}

void ArchInterrupts::enableTimer()
{
  //enableIRQ(0);
}

void ArchInterrupts::disableTimer()
{
  //disableIRQ(0);
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
  enabled = true;
  arch_enableInterrupts();
}
extern "C" void arch_disableInterrupts();
bool ArchInterrupts::disableInterrupts()
{
  arch_disableInterrupts();
  bool result = enabled;
  enabled = false;
  return result;
}

bool ArchInterrupts::testIFSet()
{
  return enabled;
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
