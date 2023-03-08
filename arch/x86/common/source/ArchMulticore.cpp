#include "ArchMulticore.h"
#include "ArchInterrupts.h"
#include "InterruptUtils.h"
#include "ProgrammableIntervalTimer.h"
#include "ArchCommon.h"
#include "Allocator.h"
#include "SMP.h"
#include "SystemState.h"
#include "debug.h"
#include "EASTL/finally.h"
#include "EASTL/atomic.h"

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
    // Physical pages 0 + 1 are used for AP startup code
    // Technically we could free them after starting all other CPUs, but a lot
    // of the rest of SWEB treats PPN == 0 as invalid (even though it's a perfectly usable page).
    // Therefore we cannot actually free them without causing problems elsewhere.
    // PPN 0 was previously used for the kernel heap and was never returned from allocPPN().
    size_t ap_boot_code_range = allocator.alloc(PAGE_SIZE*2, PAGE_SIZE);
    debug(A_MULTICORE, "Allocated mem for ap boot code: [%zx, %zx)\n",
          ap_boot_code_range, ap_boot_code_range + PAGE_SIZE*2);
    assert(ap_boot_code_range == 0);
}

void ArchMulticore::startAP(uint8_t apic_id, size_t entry_addr)
{
    debug(A_MULTICORE, "Sending init IPI to AP local APIC %u, AP entry function: %zx\n", apic_id, entry_addr);

    assert((entry_addr % PAGE_SIZE) == 0);
    assert((entry_addr/PAGE_SIZE) <= 0xFF);

    eastl::atomic_flag delay{true};
    assert(delay.test());

    ArchInterrupts::disableIRQ(PIT::instance().irq());
    auto old_pit_handler = PIT::instance().irq().handler();
    auto old_pit_mode = PIT::operatingMode();
    auto old_pit_freq = PIT::frequencyDivisor();

    // Restore old PIT state at end of function
    eastl::finally f{[&]{
        PIT::instance().irq().useHandler(old_pit_handler);
        PIT::setOperatingMode(old_pit_mode);
        PIT::setFrequencyDivisor(old_pit_freq);
    }};

    PIT::instance().irq().useHandler([&delay]{
        debugAdvanced(A_MULTICORE, "Delay interrupt called\n");
        delay.clear();
    });
    PIT::setOperatingMode(PIT::OperatingMode::ONESHOT);

    cpu_lapic->sendIPI(0, Apic::IPIDestination::TARGET, apic_id, Apic::IPIType::INIT);

    {
        // Enable PIT interrupts for this section
        ArchInterrupts::enableIRQ(PIT::instance().irq());
        eastl::finally f_disable_pit_irq{[]{ ArchInterrupts::disableIRQ(PIT::instance().irq()); }};
        WithInterrupts i{true};

        // 10ms delay
        PIT::setFrequencyDivisor(1193182 / 100);
        debugAdvanced(A_MULTICORE, "Waiting for delay 1\n");
        while(delay.test_and_set());

        cpu_lapic->sendIPI(entry_addr/PAGE_SIZE, Apic::IPIDestination::TARGET, apic_id, Apic::IPIType::SIPI);

        // 200us delay
        PIT::setFrequencyDivisor(1193182 / 100);
        debugAdvanced(A_MULTICORE, "Waiting for delay 2\n");
        while(delay.test_and_set());
    }

    // Second SIPI just in case the first one didn't work
    cpu_lapic->sendIPI(entry_addr/PAGE_SIZE, Apic::IPIDestination::TARGET, apic_id, Apic::IPIType::SIPI);

    debugAdvanced(A_MULTICORE, "Finished sending IPI to AP local APIC\n");
}
