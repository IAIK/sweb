#include "ArchInterrupts.h"

#include "8259.h"
#include "APIC.h"
#include "InterruptUtils.h"
#include "IoApic.h"
#include "KeyboardManager.h"
#include "PlatformBus.h"
#include "ProgrammableIntervalTimer.h"
#include "Scheduler.h"
#include "SegmentUtils.h"
#include "SystemState.h"
#include "Thread.h"
#include "offsets.h"
#include "ports.h"

#include "ArchCpuLocalStorage.h"
#include "ArchMemory.h"
#include "ArchMulticore.h"
#include "ArchThreads.h"

#include "assert.h"
#include "debug.h"

cpu_local IrqDomain cpu_irq_vector_domain_("CPU interrupt vector", InterruptVector::NUM_VECTORS);
cpu_local IrqDomain* cpu_root_irq_domain_ = &cpu_irq_vector_domain_;

IrqDomain& ArchInterrupts::currentCpuRootIrqDomain()
{
    assert(cpu_root_irq_domain_);
    return *cpu_root_irq_domain_;
}

IrqDomain& ArchInterrupts::isaIrqDomain()
{
    static IrqDomain isa_irq_domain("ISA IRQ", InterruptVector::NUM_ISA_INTERRUPTS);
    return isa_irq_domain;
}

static void initInterruptDescriptorTable()
{
    auto& idt = InterruptUtils::idt;
    idt.idtr().load();

    if (A_INTERRUPTS & OUTPUT_ENABLED & OUTPUT_ADVANCED)
    {
        for (size_t i = 0; i < idt.entries.size(); ++i)
        {
            debug(A_INTERRUPTS,
                  "%3zu -- offset: %p, present: %x, segment_selector: %x, "
                  "type: %x, dpl: %x\n",
                  i, idt.entries[i].offset(), idt.entries[i].present,
                  idt.entries[i].segment_selector, idt.entries[i].type,
                  idt.entries[i].dpl);
        }
    }
}

void initCpuLocalInterruptHandlers()
{
    debug(A_INTERRUPTS, "Initializing interrupt handlers\n");
    ArchInterrupts::currentCpuRootIrqDomain().irq(InterruptVector::YIELD).useHandler(int65_handler_swi_yield);
    ArchInterrupts::currentCpuRootIrqDomain().irq(InterruptVector::IPI_HALT_CPU).useHandler(int90_handler_halt_cpu);
    ArchInterrupts::currentCpuRootIrqDomain().irq(InterruptVector::APIC_ERROR).useHandler(int91_handler_APIC_error);
    ArchInterrupts::currentCpuRootIrqDomain().irq(InterruptVector::APIC_SPURIOUS).useHandler(int100_handler_APIC_spurious);
    ArchInterrupts::currentCpuRootIrqDomain().irq(InterruptVector::IPI_REMOTE_FCALL).useHandler(int101_handler_cpu_fcall);
    ArchInterrupts::currentCpuRootIrqDomain().irq(InterruptVector::SYSCALL).useHandler(syscallHandler);
}

void initInterruptControllers()
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
  initInterruptDescriptorTable();
  initInterruptControllers();
  initCpuLocalInterruptHandlers();
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
    if (cpu_lapic->isInitialized() && cpu_lapic->usingAPICTimer())
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

void ArchInterrupts::setTimerFrequency(uint32 freq)
{
  debug(A_INTERRUPTS, "Set timer frequency %u\n", freq);
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
    debugAdvanced(A_INTERRUPTS, "[Cpu %zu] Start of IRQ %u\n", SMP::currentCpuId(), irqnum);

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
    debugAdvanced(A_INTERRUPTS, "[Cpu %zu] Sending EOI for IRQ %u\n", SMP::currentCpuId(),
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
     __asm__ __volatile__("sti\n"
                          "nop\n");
}

bool ArchInterrupts::disableInterrupts()
{
   uint32 ret_val;

 __asm__ __volatile__("pushfl\n"
                      "popl %0\n"
                      "cli"
 : "=a"(ret_val));

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

struct [[gnu::packed]] context_switch_registers
{
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
};

struct [[gnu::packed]] interrupt_registers
{
    // TODO: update offsets to include interrupt num
    // w/o   | w/ error code
    uint32 eip;    // 48-51 | 52-55
    uint32 cs;     // 52-55 | 56-59
    uint32 eflags; // 56-59 | 60-63
    uint32 esp3;   // 60-63 | 64-67
    uint32 ss3;    // 64-67 | 68-71
};

struct [[gnu::packed]] SavedContextSwitchRegisters
{
    context_switch_registers registers;
    uint32_t interrupt_num;
    uint32_t error_code;
    interrupt_registers iregisters;
};

extern "C" ArchThreadRegisters*
arch_saveThreadRegisters([[maybe_unused]]uint32 error, SavedContextSwitchRegisters* saved_r)
{
    auto registers = &saved_r->registers;
    auto iregisters = &saved_r->iregisters;
    ArchThreadRegisters* info = currentThreadRegisters;

    asm("fnsave (%[fpu])\n"
        "frstor (%[fpu])\n"
        :
        : [fpu] "r"(&info->fpu));
    if ((iregisters->cs & 0x3) == 0x3)
    {
        info->ss = iregisters->ss3;
        info->esp = iregisters->esp3;
  }
  else
  {
    // Compensate for interrupt frame + error code + int num pushed onto stack before esp is saved via pusha
    info->esp = registers->esp + 0x14;
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

  return info;
}

extern "C" void genericInterruptEntry(SavedContextSwitchRegisters* regs)
{
    // Take registers previously saved on the stack via assembly and store them in the
    // saved registers of the thread
    auto saved_regs = arch_saveThreadRegisters(0, regs);

    debugAdvanced(A_INTERRUPTS, "[Cpu %zu] Interrupt entry %zu\n",
                  SMP::currentCpuId(), regs->interrupt_num);

    interruptHandler(regs->interrupt_num, regs->error_code, saved_regs);
}

extern "C" [[noreturn]] void contextSwitch(Thread* target_thread, ArchThreadRegisters* target_registers)
{
    target_thread = target_thread ? : currentThread;
    target_registers = target_registers ? : currentThreadRegisters;
    assert(target_thread);

  if(A_INTERRUPTS & OUTPUT_ADVANCED)
  {
    debug(A_INTERRUPTS, "CPU %zu, context switch to thread %p = %s, user: %u, regs: %p at eip %p, esp %p, ebp %p\n",
        SMP::currentCpuId(), target_thread, target_thread->getName(), target_thread->switch_to_userspace_, target_registers,
        (void*)target_registers->eip, (void*)target_registers->esp, (void*)target_registers->ebp);
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
  }

  assert(target_thread->isStackCanaryOK() && "Kernel stack corruption detected.");

  currentThread = target_thread;
  currentThreadRegisters = target_registers;
  currentThread->currently_scheduled_on_cpu_ = SMP::currentCpuId();

  ArchThreadRegisters info = *target_registers; // optimization: local copy produces more efficient code in this case
  if (target_thread->switch_to_userspace_)
  {
    asm("push %[ss]" : : [ss]"m"(info.ss));
    asm("push %[esp]" : : [esp]"m"(info.esp));
  }
  else
  {
    asm("mov %[esp], %%esp\n" : : [esp]"m"(info.esp));
  }
  cpu_tss.setTaskStack(info.esp0);
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
