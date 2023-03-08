#include "ArchMulticore.h"
#include "ArchInterrupts.h"
#include "InterruptUtils.h"
#include "ProgrammableIntervalTimer.h"
#include "ArchCommon.h"
#include "Allocator.h"
#include "SMP.h"
#include "SystemState.h"
#include "debug.h"

extern eastl::atomic<bool> ap_started;

ArchCpu::ArchCpu() :
    lapic(cpu_lapic)
{
    setId(lapic->isInitialized() ? lapic->apicId() : 0);
    debug(A_MULTICORE, "Initializing ArchCpu %zu\n", id());
    SMP::addCpuToList(this);
    root_domain_ptr = &cpu_root_irq_domain_;
}

IrqDomain& ArchCpu::rootIrqDomain()
{
    assert(*root_domain_ptr);
    return **root_domain_ptr;
}

void ArchMulticore::notifyMessageAvailable(ArchCpu& cpu)
{
    cpu_lapic->sendIPI(MESSAGE_INT_VECTOR, *cpu.lapic, true);
}

void ArchMulticore::sendFunctionCallMessage(ArchCpu& cpu, RemoteFunctionCallMessage* fcall_message)
{
    cpu.enqueueFunctionCallMessage(fcall_message);
    notifyMessageAvailable(cpu);
}

void ArchMulticore::stopAllCpus()
{
    if(CpuLocalStorage::ClsInitialized() && cpu_lapic->isInitialized())
    {
        cpu_lapic->sendIPI(STOP_INT_VECTOR);
    }
}

void ArchMulticore::stopOtherCpus()
{
    if(CpuLocalStorage::ClsInitialized() && cpu_lapic->isInitialized())
    {
        cpu_lapic->sendIPI(STOP_INT_VECTOR, Apic::IPIDestination::OTHERS);
    }
}

[[noreturn]] void ArchMulticore::waitForSystemStart()
{
    kprintf("CPU %zu initialized, waiting for system start\n", SMP::currentCpuId());
    debug(A_MULTICORE, "CPU %zu initialized, waiting for system start\n", SMP::currentCpuId());
    assert(CpuLocalStorage::ClsInitialized());
    ap_started = true;

    while(system_state != RUNNING);

    ArchInterrupts::enableInterrupts();

    while(true)
    {
        debug(A_MULTICORE, "CPU %zu halting\n", SMP::currentCpuId());
        ArchCommon::halt();
    }
    assert(false);
}

char* ArchMulticore::cpuStackTop()
{
    return cpu_stack + sizeof(cpu_stack);
}

void ArchMulticore::reservePages(Allocator& allocator)
{
    // HACKY: Pages 0 + 1 are used for AP startup code
    size_t ap_boot_code_range = allocator.alloc(PAGE_SIZE*2, PAGE_SIZE);
    debug(A_MULTICORE, "Allocated mem for ap boot code: [%zx, %zx)\n",
          ap_boot_code_range, ap_boot_code_range + PAGE_SIZE*2);
    assert(ap_boot_code_range == 0);
}

static volatile uint8_t delay = 0;

#ifdef CMAKE_X86_64
#define IRET __asm__ __volatile__("iretq\n");
#else
#define IRET __asm__ __volatile__("iretl\n");
#endif

extern "C" void PIT_delay_IRQ();
__attribute__((naked)) void __PIT_delay_IRQ()
{
    __asm__ __volatile__(".global PIT_delay_IRQ\n"
                         ".type PIT_delay_IRQ,@function\n"
                         "PIT_delay_IRQ:\n"
                         "movb $1, %[delay]\n"
                         :[delay]"=m"(delay));
    IRET
}

void ArchMulticore::startAP(uint8_t apic_id, size_t entry_addr)
{
    debug(A_MULTICORE, "Sending init IPI to AP local APIC %u, AP entry function: %zx\n",  apic_id, entry_addr);

    assert((entry_addr % PAGE_SIZE) == 0);
    assert((entry_addr/PAGE_SIZE) <= 0xFF);

    cpu_lapic->sendIPI(0, Apic::IPIDestination::TARGET, apic_id, Apic::IPIType::INIT);

    // 10ms delay

    InterruptGateDesc temp_irq0_descriptor = InterruptUtils::idt.entries[InterruptVector::REMAP_OFFSET + (uint8_t)ISA_IRQ::PIT];
    bool temp_using_apic_timer = cpu_lapic->usingAPICTimer();
    bool apic_timer_mask = cpu_lapic->timer_interrupt_controller.isMasked();
    ArchInterrupts::disableIRQ(PIT::instance().irq());

    if (temp_using_apic_timer)
    {
        if (!apic_timer_mask)
        {
            SMP::currentCpu().rootIrqDomain().activateIrq(InterruptVector::APIC_TIMER, false);
            assert(cpu_lapic->timer_interrupt_controller.isMasked());
        }
        cpu_lapic->setUsingAPICTimer(false);
    }

    auto old_pit_mode = PIT::setOperatingMode(PIT::OperatingMode::ONESHOT);
    auto old_pit_freq = PIT::frequencyDivisor();

    InterruptUtils::idt.entries[InterruptVector::REMAP_OFFSET + (uint8_t)ISA_IRQ::PIT].setOffset(&PIT_delay_IRQ);

    ArchInterrupts::enableIRQ(PIT::instance().irq());
    ArchInterrupts::startOfInterrupt(InterruptVector::REMAP_OFFSET + (uint8_t)ISA_IRQ::PIT);
    ArchInterrupts::enableInterrupts();

    PIT::setOperatingMode(PIT::OperatingMode::ONESHOT);
    PIT::setFrequencyDivisor(1193182 / 100);
    debugAdvanced(A_MULTICORE, "Waiting for delay 1\n");
    assert(ArchInterrupts::testIFSet());
    while(!delay);

    ArchInterrupts::disableInterrupts();
    ArchInterrupts::disableIRQ(PIT::instance().irq());
    ArchInterrupts::endOfInterrupt(InterruptVector::REMAP_OFFSET + (uint8_t)ISA_IRQ::PIT);

    delay = 0;

    cpu_lapic->sendIPI(entry_addr/PAGE_SIZE, Apic::IPIDestination::TARGET, apic_id, Apic::IPIType::SIPI);

    // 200us delay

    ArchInterrupts::enableIRQ(PIT::instance().irq());
    ArchInterrupts::startOfInterrupt(InterruptVector::REMAP_OFFSET + (uint8_t)ISA_IRQ::PIT);
    ArchInterrupts::enableInterrupts();

    PIT::setFrequencyDivisor(1193182 / 100);
    debugAdvanced(A_MULTICORE, "Waiting for delay 2\n");
    while(!delay);

    ArchInterrupts::disableInterrupts();
    ArchInterrupts::disableIRQ(PIT::instance().irq());
    ArchInterrupts::endOfInterrupt(InterruptVector::REMAP_OFFSET + (uint8_t)ISA_IRQ::PIT);

    delay = 0;

    // Second SIPI just in case the first one didn't work
    cpu_lapic->sendIPI(entry_addr/PAGE_SIZE, Apic::IPIDestination::TARGET, apic_id, Apic::IPIType::SIPI);

    if (temp_using_apic_timer)
    {
        cpu_lapic->setUsingAPICTimer(temp_using_apic_timer);
        SMP::currentCpu().rootIrqDomain().activateIrq(InterruptVector::APIC_TIMER, !apic_timer_mask);
    }

    PIT::setOperatingMode(old_pit_mode);
    PIT::setFrequencyDivisor(old_pit_freq);

    InterruptUtils::idt.entries[InterruptVector::REMAP_OFFSET + (uint8_t)ISA_IRQ::PIT] = temp_irq0_descriptor;

    debugAdvanced(A_MULTICORE, "Finished sending IPI to AP local APIC\n");
}
