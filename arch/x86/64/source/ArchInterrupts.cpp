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
#include "IrqDomain.h"
#include "PlatformBus.h"
#include "KeyboardManager.h"

cpu_local IrqDomain cpu_irq_vector_domain_("CPU interrupt vector");
cpu_local IrqDomain* cpu_root_irq_domain_ = &cpu_irq_vector_domain_;

IrqDomain& ArchInterrupts::currentCpuRootIrqDomain()
{
    assert(cpu_root_irq_domain_);
    return *cpu_root_irq_domain_;
}

IrqDomain& ArchInterrupts::isaIrqDomain()
{
    static IrqDomain isa_irq_domain("ISA IRQ");
    return isa_irq_domain;
}

static void initInterruptHandlers()
{
  debug(A_INTERRUPTS, "Initializing interrupt handlers\n");
  InterruptUtils::initialise();
}

static void initInterruptController()
{
  debug(A_INTERRUPTS, "Initializing interrupt controllers\n");
  assert(CpuLocalStorage::ClsInitialized());

  PlatformBus::instance().registerDriver(ApicDriver::instance());
  PlatformBus::instance().registerDriver(ApicTimerDriver::instance());
  PlatformBus::instance().registerDriver(IoApicDriver::instance());
  PlatformBus::instance().registerDriver(PIC8259Driver::instance());

  assert(cpu_root_irq_domain_);
  debug(A_INTERRUPTS, "Interrupt controllers initialized\n");
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
    debug(A_INTERRUPTS, "Enabling xApic %x timer \n", cpu_lapic->apicId());
    enableIRQ(cpu_lapic->timer_interrupt_controller.irq());
  }
  else
  {
    debug(A_INTERRUPTS, "Enabling PIT timer IRQ\n");
    enableIRQ(PIT::instance().irq());
  }
}

void ArchInterrupts::disableTimer()
{
  if(cpu_lapic->isInitialized() && cpu_lapic->usingAPICTimer())
  {
    debug(A_INTERRUPTS, "Disabling xApic %x timer \n", cpu_lapic->apicId());
    disableIRQ(cpu_lapic->timer_interrupt_controller.irq());
  }
  else
  {
    debug(A_INTERRUPTS, "Disabling PIT timer IRQ\n");
    disableIRQ(PIT::instance().irq());
  }
}

void ArchInterrupts::setTimerFrequency(uint32 freq) {
  debug(A_INTERRUPTS, "Set timer frequency %u\n", freq);

  PIT::setOperatingMode(PIT::OperatingMode::SQUARE_WAVE);

  uint16_t divisor;
  if(freq < (uint32)(1193180. / (1 << 16) + 1)) {
    divisor = 0;
  } else {
    divisor = (uint16)(1193180 / freq);
  }

  PIT::setFrequencyDivisor(divisor);
}


void ArchInterrupts::enableKBD()
{
    debug(A_INTERRUPTS, "Enable keyboard irq\n");

    enableIRQ(KeyboardManager::instance().irq());
}

void ArchInterrupts::disableKBD()
{
    enableIRQ(KeyboardManager::instance().irq(), false);
}

void ArchInterrupts::enableIRQ(const IrqDomain::DomainIrqHandle& irq_handle, bool enable)
{
    debug(A_INTERRUPTS, "[Cpu %zu] %s %s IRQ %zx\n", SMP::currentCpuId(),
          enable ? "Enable" : "Disable", irq_handle.domain().name().c_str(),
          irq_handle.irq());

    for (auto&& domain_irq : irq_handle.forwardMappingChain())
    {
        domain_irq.activateInDomain(enable);
    }
}

void ArchInterrupts::disableIRQ(const IrqDomain::DomainIrqHandle& irq_handle)
{
    enableIRQ(irq_handle, false);
}

// void ArchInterrupts::enableIRQ(uint16 irqnum, bool enable)
// {
//     debug(A_INTERRUPTS, "[Cpu %zu] %s IRQ %x\n", SMP::currentCpuId(),
//           enable ? "Enable" : "Disable", irqnum);

//     enableIRQ(currentCpuRootIrqDomain().irq(irqnum), enable);
// }

// void ArchInterrupts::disableIRQ(uint16 irqnum)
// {
//     enableIRQ(irqnum, false);
// }

void ArchInterrupts::startOfInterrupt(const IrqDomain::DomainIrqHandle& irq_handle)
{
    for (auto&& [domain, local_irqnum] : irq_handle.reverseMappingTree())
    {
        if (domain->controller())
        {
            domain->controller()->irqStart(local_irqnum);
        }
    }
}

void ArchInterrupts::startOfInterrupt(uint16 irqnum)
{
    debugAdvanced(A_INTERRUPTS, "[Cpu %zu] Start of IRQ %x\n", SMP::currentCpuId(), irqnum);

    startOfInterrupt(currentCpuRootIrqDomain().irq(irqnum));
}

void ArchInterrupts::endOfInterrupt(const IrqDomain::DomainIrqHandle& irq_handle)
{
    for (auto&& [domain, local_irqnum] : irq_handle.reverseMappingTree())
    {
        if (domain->controller())
        {
            domain->controller()->ack(local_irqnum);
        }
    }
}

void ArchInterrupts::endOfInterrupt(uint16 irqnum)
{
    debugAdvanced(A_INTERRUPTS, "[Cpu %zu] Sending EOI for IRQ %x\n", SMP::currentCpuId(),
                  irqnum);

    endOfInterrupt(currentCpuRootIrqDomain().irq(irqnum));
}

void ArchInterrupts::handleInterrupt(const IrqDomain::DomainIrqHandle& irq_handle)
{
    for (auto&& domain_irq : irq_handle.reverseMappingTree())
    {
        domain_irq.handleInDomain();
    }
}

void ArchInterrupts::handleInterrupt(uint16_t irqnum)
{
    handleInterrupt(currentCpuRootIrqDomain().irq(irqnum));
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
      debug(A_INTERRUPTS, "CPU %zu, context switch to thread %s (%p) at rip %p\n", SMP::currentCpuId(), target_thread->getName(), target_thread, (void*)target_registers->rip);
  }


  assert(target_registers);
  assert(!currentThread || currentThread->currently_scheduled_on_cpu_ == SMP::currentCpuId());

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
