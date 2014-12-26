/**
 * @file ArchInterrupts.cpp
 *
 */

#include "ArchInterrupts.h"
#include "8259.h"
#include "ports.h"
#include "InterruptUtils.h"
#include "ArchThreads.h"

void ArchInterrupts::initialise()
{
  uint16 i;
  disableInterrupts();
  initialise8259s();
  InterruptUtils::initialise();
  for (i=0;i<16;++i)
    disableIRQ(i);
}

void ArchInterrupts::enableTimer()
{
  enableIRQ(0);
}

void ArchInterrupts::disableTimer()
{
  disableIRQ(0);
}

void ArchInterrupts::enableKBD()
{
  enableIRQ(1);
  enableIRQ(9);
}

void ArchInterrupts::disableKBD()
{
  disableIRQ(1);
}

void ArchInterrupts::EndOfInterrupt(uint16 number) 
{
  sendEOI(number);
}

void ArchInterrupts::enableInterrupts()
{
     __asm__ __volatile__("sti"
   :
   :
   );
}

bool ArchInterrupts::disableInterrupts()
{
   uint64 ret_val;

 __asm__ __volatile__("pushfq\n"
                      "popq %0\n"
                      "cli"
 : "=a"(ret_val)
 :);

return (ret_val & (1 << 9));  //testing IF Flag

}

bool ArchInterrupts::testIFSet()
{
  uint64 ret_val;

  __asm__ __volatile__(
  "pushfq\n"
  "popq %0\n"
  : "=a"(ret_val)
  :);

  return (ret_val & (1 << 9));  //testing IF Flag
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


struct context_switch_registers {
  uint64 ds;
  uint64 es;
  uint64 r15;
  uint64 r14;
  uint64 r13;
  uint64 r12;
  uint64 r11;
  uint64 r10;
  uint64 r9;
  uint64 r8;
  uint64 rdi;
  uint64 rsi;
  uint64 rbp;
  uint64 rbx;
  uint64 rdx;
  uint64 rcx;
  uint64 rax;
  uint64 rsp;
};

struct interrupt_registers {
  uint64 rip;
  uint64 cs;
  uint64 rflags;
  uint64 rsp;
  uint64 ss;
};

#include "kprintf.h"

extern "C" void arch_saveThreadRegistersGeneric(uint64* base, uint64 error)
{
  register struct context_switch_registers* registers;
  registers = (struct context_switch_registers*) base;
  register struct interrupt_registers* iregisters;
  iregisters = (struct interrupt_registers*) (base + sizeof(struct context_switch_registers)/sizeof(uint64) + error);
  register ArchThreadInfo* info = currentThreadInfo;
  asm("fnsave (%0)\n"
      "frstor (%0)\n"
      :
      : "r"((void*)(&(info->fpu)))
      :);
  info->rsp = iregisters->rsp;
  info->rip = iregisters->rip;
  info->cs = iregisters->cs;
  info->rflags = iregisters->rflags;
  info->es = registers->es;
  info->ds = registers->ds;
  info->r15 = registers->r15;
  info->r14 = registers->r14;
  info->r13 = registers->r13;
  info->r12 = registers->r12;
  info->r11 = registers->r11;
  info->r10 = registers->r10;
  info->r9 = registers->r9;
  info->r8 = registers->r8;
  info->rdi = registers->rdi;
  info->rsi = registers->rsi;
  info->rbx = registers->rbx;
  info->rdx = registers->rdx;
  info->rcx = registers->rcx;
  info->rax = registers->rax;
  info->rbp = registers->rbp;
}

extern "C" void arch_saveThreadRegisters(uint64* base)
{
  arch_saveThreadRegistersGeneric(base, 0);
}

extern "C" void arch_saveThreadRegistersForPageFault(uint64* base)
{
  arch_saveThreadRegistersGeneric(base, 1);
}

