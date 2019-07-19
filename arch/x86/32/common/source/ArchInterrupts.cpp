#include "ArchInterrupts.h"
#include "8259.h"
#include "ports.h"
#include "InterruptUtils.h"
#include "SegmentUtils.h"
#include "ArchThreads.h"
#include "assert.h"
#include "Thread.h"
#include "debug.h"


static void initInterruptHandlers()
{
  InterruptUtils::initialise();
}

static void initInterruptController()
{
  uint16 i;
  initialise8259s();
  for (i=0;i<16;++i)
          disableIRQ(i);
}

void ArchInterrupts::initialise()
{
  initInterruptHandlers();
  initInterruptController();
}



void ArchInterrupts::enableTimer()
{
  enableIRQ(0);
}

void ArchInterrupts::setTimerFrequency(uint32 freq)
{
  uint16_t divisor;
  if(freq < (uint32)(1193180. / (1 << 16) + 1)) {
    divisor = 0;
  } else {
    divisor = (uint16)(1193180 / freq);
  }
  outportb(0x43, 0x36);
  outportb(0x40, divisor & 0xFF);
  outportb(0x40, divisor >> 8);
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

void ArchInterrupts::endOfInterrupt(uint16 number)
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
   uint32 ret_val;

 __asm__ __volatile__("pushfl\n"
                      "popl %0\n"
                      "cli"
 : "=a"(ret_val)
 :);

return (ret_val & (1 << 9));  //testing IF Flag

}

bool ArchInterrupts::testIFSet()
{
  uint32 ret_val;

  __asm__ __volatile__(
  "pushfl\n"
  "popl %0\n"
  : "=a"(ret_val));

  return (ret_val & (1 << 9));  //testing IF Flag
}

void ArchInterrupts::yieldIfIFSet()
{
  if (system_state == RUNNING && currentThread && testIFSet())
  {
    ArchThreads::yield();
  }
  else
  {
    __asm__ __volatile__("nop");
  }
}

struct context_switch_registers {
  uint32 gs;  //  0-3
  uint32 fs;  //  4-7
  uint32 es;  //  8-11
  uint32 ds;  // 12-15
  uint32 edi; // 16-19
  uint32 esi; // 20-23
  uint32 ebp; // 24-27
  uint32 esp; // 28-31
  uint32 ebx; // 32-35
  uint32 edx; // 36-39
  uint32 ecx; // 40-43
  uint32 eax; // 44-47
} __attribute__((packed));

struct interrupt_registers {
                 // w/o   | w/ error code
  uint32 eip;    // 48-51 | 52-55
  uint32 cs;     // 52-55 | 56-59
  uint32 eflags; // 56-59 | 60-63
  uint32 esp3;   // 60-63 | 64-67
  uint32 ss3;    // 64-67 | 68-71
} __attribute__((packed));


extern "C" void arch_dummyHandler();
extern "C" void arch_dummyHandlerMiddle();
extern "C" size_t arch_computeDummyHandler(uint32 eip)
{
  size_t dummy_handler_sled_size = (((size_t) arch_dummyHandlerMiddle) - (size_t) arch_dummyHandler);
  assert((dummy_handler_sled_size % 128) == 0 && "cannot handle weird padding in the kernel binary");
  dummy_handler_sled_size /= 128;
  assert((eip <= (size_t) arch_dummyHandlerMiddle) && "calling dummy handler cannot be outside of dummy handler sled");
  assert((eip >= (size_t) arch_dummyHandler) && "calling dummy handler cannot be outside of dummy handler sled");
  size_t calling_dummy_handler = (eip - (size_t) arch_dummyHandler) / dummy_handler_sled_size - 1;
  return calling_dummy_handler;
}

extern "C" void arch_saveThreadRegisters(uint32 error, uint32* base)
{
  struct context_switch_registers* registers = (struct context_switch_registers*) base;
  struct interrupt_registers* iregisters = (struct interrupt_registers*) ((size_t)(registers + 1) + error*sizeof(uint32));
  ArchThreadRegisters* info = currentThreadRegisters;

  asm("fnsave (%[fpu])\n"
      "frstor (%[fpu])\n"
      :
      : [fpu]"r"(&info->fpu));
  if ((iregisters->cs & 0x3) == 0x3)
  {
    info->ss = iregisters->ss3;
    info->esp = iregisters->esp3;
  }
  else
  {
    info->esp = registers->esp + 0xc;
  }
  info->eip = iregisters->eip;
  info->cs = iregisters->cs;
  info->eflags = iregisters->eflags;
  info->eax = registers->eax;
  info->ecx = registers->ecx;
  info->edx = registers->edx;
  info->ebx = registers->ebx;
  info->ebp = registers->ebp;
  info->esi = registers->esi;
  info->edi = registers->edi;
  info->ds = registers->ds;
  info->es = registers->es;
  info->fs = registers->fs;
  info->gs = registers->gs;
  assert(!currentThread || currentThread->isStackCanaryOK());
}

extern TSS *g_tss;

extern "C" void arch_contextSwitch()
{
  assert(currentThread->isStackCanaryOK() && "Kernel stack corruption detected.");
  if(outstanding_EOIs)
  assert(currentThread);
  {
          debug(A_INTERRUPTS, "%zu outstanding End-Of-Interrupt signal(s) on context switch. Probably called yield in the wrong place (e.g. in the scheduler/IRQ0)\n", outstanding_EOIs);
          assert(!outstanding_EOIs);
  }
  ArchThreadRegisters info = *currentThreadRegisters; // optimization: local copy produces more efficient code in this case
  if (currentThread->switch_to_userspace_)
  {
    assert(currentThread->holding_lock_list_ == 0 && "Never switch to userspace when holding a lock! Never!");
    assert(currentThread->lock_waiting_on_ == 0 && "How did you even manage to execute code while waiting for a lock?");
    asm("push %[ss]" : : [ss]"m"(info.ss));
    asm("push %[esp]" : : [esp]"m"(info.esp));
  }
  else
  {
    asm("mov %[esp], %%esp\n" : : [esp]"m"(info.esp));
  }
  g_tss->esp0 = info.esp0;
  asm("frstor (%[fpu])\n" : : [fpu]"r"(&info.fpu));
  asm("mov %[cr3], %%cr3\n" : : [cr3]"r"(info.cr3));
  asm("push %[eflags]\n" : : [eflags]"m"(info.eflags));
  asm("push %[cs]\n" : : [cs]"m"(info.cs));
  asm("push %[eip]\n" : : [eip]"m"(info.eip));
  asm("mov %[esi], %%esi\n" : : [esi]"m"(info.esi));
  asm("mov %[edi], %%edi\n" : : [edi]"m"(info.edi));
  asm("mov %[es], %%es\n" : : [es]"m"(info.es));
  asm("mov %[ds], %%ds\n" : : [ds]"m"(info.ds));
  asm("mov %[fs], %%fs\n" : : [fs]"m"(info.fs));
  asm("mov %[gs], %%gs\n" : : [gs]"m"(info.gs)); // Don't use CPU local storage after loading %gs
  asm("push %[ebp]\n" : : [ebp]"m"(info.ebp));
  asm("pop %%ebp\n"
      "iret" : : "a"(info.eax), "b"(info.ebx), "c"(info.ecx), "d"(info.edx));
  asm("hlt");
  assert(false);
}
