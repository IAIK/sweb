#include "ArchInterrupts.h"
#include "8259.h"
#include "ProgrammableIntervalTimer.h"
#include "ports.h"
#include "InterruptUtils.h"
#include "ArchThreads.h"
#include "assert.h"
#include "Thread.h"
#include "APIC.h"
#include "IoApic.h"
#include "debug.h"
#include "ArchMemory.h"
#include "PageManager.h"
#include "ArchMulticore.h"
#include "Scheduler.h"
#include "SystemState.h"
#include "X2Apic.h"
#include "CPUID.h"

static void initInterruptHandlers()
{
  debug(A_INTERRUPTS, "Initializing interrupt handlers\n");
  InterruptUtils::initialise();
}

void setIMCRMode(IMCRData mode)
{
    // Intel MultiProcessor Specification chapter 3.6.2
    // https://pdos.csail.mit.edu/6.828/2008/readings/ia32/MPspec.pdf
    debug(A_INTERRUPTS, "Ensure IMCR is set to APIC passthrough/symmetric mode\n");
    // IMCR register might not actually exist, but attempting to write to it should be fine?
    IMCR_SELECT::write(IMCRSelect::SELECT_IMCR);
    IMCR_DATA::write(mode);
}

static void initInterruptController()
{
  debug(A_INTERRUPTS, "Initializing interrupt controllers\n");
  assert(CpuLocalStorage::ClsInitialized());
  if (cpu_features.cpuHasFeature(CpuFeatures::APIC))
  {
      setIMCRMode(IMCRData::APIC_PASSTHROUGH);

      Apic::globalEnable();
      if (X2Apic::x2ApicSupported())
      {
          X2Apic::enableX2ApicMode();
      }
      else
      {
          XApic::setPhysicalAddress(XApic::readMsrPhysAddr());
          XApic::mapAt((size_t)XApic::physicalAddress() | PHYSICAL_TO_VIRTUAL_OFFSET);
      }

      cpu_lapic->init();
  }

  IOAPIC::initAll();

  PIC8259::initialise8259s();
}

void ArchInterrupts::initialise()
{
  initInterruptHandlers();
  initInterruptController();
}

void ArchInterrupts::enableTimer()
{
  if(cpu_lapic->isInitialized() && cpu_lapic->usingAPICTimer())
  {
    debug(A_INTERRUPTS, "Enabling XApic %x timer \n", cpu_lapic->Id());
    WithInterrupts i{false};
    auto timer_reg = cpu_lapic->readRegister<Apic::Register::LVT_TIMER>();
    timer_reg.setMask(false);
    cpu_lapic->writeRegister<Apic::Register::LVT_TIMER>(timer_reg);
  }
  else
  {
    debug(A_INTERRUPTS, "Enabling PIT timer IRQ\n");
    ArchInterrupts::enableIRQ(0);
  }
}

void ArchInterrupts::disableTimer()
{
  if(cpu_lapic->isInitialized() && cpu_lapic->usingAPICTimer())
  {
    debug(A_INTERRUPTS, "Enabling XApic %x timer \n", cpu_lapic->Id());
    WithInterrupts i{false};
    auto timer_reg = cpu_lapic->readRegister<Apic::Register::LVT_TIMER>();
    timer_reg.setMask(true);
    cpu_lapic->writeRegister<Apic::Register::LVT_TIMER>(timer_reg);
  }
  else
  {
    debug(A_INTERRUPTS, "Disabling PIT timer IRQ\n");
    disableIRQ(0);
  }
}

void ArchInterrupts::setTimerFrequency(uint32 freq) {
  debug(A_INTERRUPTS, "Set timer frequency %u\n", freq);
  uint16_t divisor;
  if(freq < (uint32)(1193180. / (1 << 16) + 1)) {
    divisor = 0;
  } else {
    divisor = (uint16)(1193180 / freq);
  }

  PIT::PITCommandRegister command{};
  command.bcd_mode = 0;
  command.operating_mode = 3; // square wave generator
  command.access_mode = 3; // send low + high byte of reload value/divisor
  command.channel = 0;

  PIT::init(command.value, divisor);
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

void ArchInterrupts::enableIRQ(uint16 num)
{
  if(IOAPIC::findIOAPICforIRQ(num))
  {
    IOAPIC::setIRQMask(num, false);
  }
  else
  {
    PIC8259::enableIRQ(num);
  }
}

void ArchInterrupts::disableIRQ(uint16 num)
{
  if(IOAPIC::findIOAPICforIRQ(num))
  {
    IOAPIC::setIRQMask(num, true);
  }
  else
  {
    PIC8259::disableIRQ(num);
  }
}

void ArchInterrupts::startOfInterrupt(uint16 number)
{
  if(cpu_lapic->isInitialized() &&
     (IOAPIC::findIOAPICforIRQ(number) ||
      ((number == 0) && cpu_lapic->usingAPICTimer()) ||
      (number > 16)))
  {
    cpu_lapic->outstanding_EOIs_++;
  }
  else
  {
    PIC8259::outstanding_EOIs_++;
  }
}

void ArchInterrupts::endOfInterrupt(uint16 number)
{
    if(A_INTERRUPTS & OUTPUT_ADVANCED) {
        debug(A_INTERRUPTS, "Sending EOI for IRQ %x\n", number);
    }

  if(cpu_lapic->isInitialized() &&
     (IOAPIC::findIOAPICforIRQ(number) ||
      ((number == 0) && cpu_lapic->usingAPICTimer()) ||
      (number > 16)))
  {
    cpu_lapic->sendEOI(number + 0x20);
  }
  else
  {
    PIC8259::sendEOI(number);
  }
}

void ArchInterrupts::enableInterrupts()
{
   asm("sti\n"
       "nop\n");
}

bool ArchInterrupts::disableInterrupts()
{
  uint64 ret_val;
  asm("pushfq\n"
      "popq %0\n"
      "cli"
      : "=a"(ret_val));
  bool previous_state = (ret_val & (1 << 9));
  return previous_state;  //testing IF Flag
}

bool ArchInterrupts::testIFSet()
{
  uint64 ret_val;

  asm("pushfq\n"
      "popq %0\n"
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
    asm("nop");
  }
}


struct context_switch_registers {
  uint64 fsbase_low;
  uint64 fsbase_high;
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

struct SavedContextSwitchRegisters
{
    context_switch_registers registers;
    interrupt_registers iregisters;
} __attribute__((packed));

struct SavedContextSwitchRegistersWithError
{
    context_switch_registers registers;
    uint64 error;
    interrupt_registers iregisters;
} __attribute__((packed));


extern "C" void arch_saveThreadRegisters(void* base, uint64 error)
{
  context_switch_registers* registers = error ? &((SavedContextSwitchRegistersWithError*)base)->registers :
                                                &((SavedContextSwitchRegisters*)base)->registers;
  interrupt_registers* iregisters = error ? &((SavedContextSwitchRegistersWithError*)base)->iregisters :
                                            &((SavedContextSwitchRegisters*)base)->iregisters;

  restoreSavedFSBase();
  ArchThreadRegisters* info = currentThreadRegisters;
  asm("fnsave %[fpu]\n"
      "frstor %[fpu]\n"
      :
      : [fpu]"m"((info->fpu))
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
  info->fsbase = (registers->fsbase_high << 32) | registers->fsbase_low;
  assert(!currentThread || currentThread->isStackCanaryOK());
}

extern "C" [[noreturn]] void contextSwitch(Thread* target_thread, ArchThreadRegisters* target_registers)
{
  target_thread = target_thread ? : currentThread;
  target_registers = target_registers ? : currentThreadRegisters;
  assert(target_thread);

  if(A_INTERRUPTS & OUTPUT_ADVANCED)
  {
    debug(A_INTERRUPTS, "CPU %zu, context switch to thread %p = %s at rip %p\n", SMP::currentCpuId(), target_thread, target_thread->getName(), (void*)target_registers->rip);
    //Scheduler::instance()->printThreadList();
  }


  assert(target_registers);
  if (currentThread)
  {
      assert(currentThread->currently_scheduled_on_cpu_ == SMP::currentCpuId());
  }

  if((SMP::currentCpuId() == 0) && PIC8259::outstanding_EOIs_) // TODO: Check local APIC for pending EOIs
  {
    debug(A_INTERRUPTS, "%zu pending End-Of-Interrupt signal(s) on context switch. Probably called yield in the wrong place (e.g. in the scheduler)\n", PIC8259::outstanding_EOIs_);
    assert(!((SMP::currentCpuId() == 0) && PIC8259::outstanding_EOIs_));
  }
  if (target_thread->switch_to_userspace_)
  {
      assert(target_thread->holding_lock_list_ == 0 && "Never switch to userspace when holding a lock! Never!");
      assert(target_thread->lock_waiting_on_ == 0 && "How did you even manage to execute code while waiting for a lock?");
      assert((target_registers->cs & 3) == 3 && "Incorrect ring level for switch to userspace");
  }
  assert(target_thread->isStackCanaryOK() && "Kernel stack corruption detected.");

  currentThread = target_thread;
  currentThreadRegisters = target_registers;
  currentThread->currently_scheduled_on_cpu_ = SMP::currentCpuId();

  ArchThreadRegisters info = *target_registers;
  assert(info.rip >= PAGE_SIZE); // debug
  assert(info.rsp0 >= USER_BREAK);
  cpu_tss.setTaskStack(info.rsp0);
  size_t new_fsbase = target_thread->switch_to_userspace_ ? target_registers->fsbase : (uint64)getSavedFSBase();
  setFSBase(new_fsbase); // Don't use CLS after this line
  asm("frstor %[fpu]\n" : : [fpu]"m"(info.fpu));
  asm("mov %[cr3], %%cr3\n" : : [cr3]"r"(info.cr3));
  asm("push %[ss]" : : [ss]"m"(info.ss));
  asm("push %[rsp]" : : [rsp]"m"(info.rsp));
  asm("push %[rflags]\n" : : [rflags]"m"(info.rflags));
  asm("push %[cs]\n" : : [cs]"m"(info.cs));
  asm("push %[rip]\n" : : [rip]"m"(info.rip));
  asm("mov %[rsi], %%rsi\n" : : [rsi]"m"(info.rsi));
  asm("mov %[rdi], %%rdi\n" : : [rdi]"m"(info.rdi));
  asm("mov %[es], %%es\n" : : [es]"m"(info.es));
  asm("mov %[ds], %%ds\n" : : [ds]"m"(info.ds));
  asm("mov %[r8], %%r8\n" : : [r8]"m"(info.r8));
  asm("mov %[r9], %%r9\n" : : [r9]"m"(info.r9));
  asm("mov %[r10], %%r10\n" : : [r10]"m"(info.r10));
  asm("mov %[r11], %%r11\n" : : [r11]"m"(info.r11));
  asm("mov %[r12], %%r12\n" : : [r12]"m"(info.r12));
  asm("mov %[r13], %%r13\n" : : [r13]"m"(info.r13));
  asm("mov %[r14], %%r14\n" : : [r14]"m"(info.r14));
  asm("mov %[r15], %%r15\n" : : [r15]"m"(info.r15));
  asm("mov %[rdx], %%rdx\n" : : [rdx]"m"(info.rdx));
  asm("mov %[rcx], %%rcx\n" : : [rcx]"m"(info.rcx));
  asm("mov %[rbx], %%rbx\n" : : [rbx]"m"(info.rbx));
  asm("mov %[rax], %%rax\n" : : [rax]"m"(info.rax));
  asm("mov %[rbp], %%rbp\n" : : [rbp]"m"(info.rbp));
  // Check %cs in iret frame on stack whether we're returning to userspace
  asm("testl $3, 8(%rsp)\n"
      "jz 1f\n"
      "swapgs\n"
      "1: iretq\n");
  assert(false);
}
