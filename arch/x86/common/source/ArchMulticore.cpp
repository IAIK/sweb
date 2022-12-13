#include "ArchMulticore.h"
#include "ArchInterrupts.h"
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

    while(1)
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
