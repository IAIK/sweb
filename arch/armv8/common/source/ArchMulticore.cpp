#include "ArchMulticore.h"

#include "SMP.h"

#include "ArchCpuLocalStorage.h"
#include "ArchInterrupts.h"

#include "EASTL/atomic.h"

#include "debug.h"

extern eastl::atomic<size_t> running_cpus;

size_t readCpuIdRegister()
{
    uint64_t mpidr_el1 = 0;
    asm("MRS %[mpidr_el1], MPIDR_EL1\n"
        :[mpidr_el1]"=g"(mpidr_el1));
    return mpidr_el1 & 0xFF;
}

ArchCpu::ArchCpu()
{
    setId(readCpuIdRegister());
    debug(A_MULTICORE, "Initializing ArchCpu %zx\n", id());
    SMP::addCpuToList(this);
}

IrqDomain& ArchCpu::rootIrqDomain()
{
    if (static bool initialized = false; !initialized)
    {
        initialized = true;
        new (&cpu_irq_vector_domain_) IrqDomain("CPU interrupt vector");
    }

    return cpu_irq_vector_domain_;
}

void ArchMulticore::initialize()
{
    assert(running_cpus == 0);
    running_cpus = 1;
    CpuLocalStorage::setCls(CpuLocalStorage::allocCls());
    ArchMulticore::initCpuLocalData(true);
}

void ArchMulticore::initCpuLocalData([[maybe_unused]]bool boot_cpu)
{
    // The constructor of objects declared as cpu_local will be called automatically
    // the first time the cpu_local object is used.
    // cpu_local isn't actually cpu_local on armv8 (no cls implemented) -> initialized via global
    // constructors
    debug(A_MULTICORE, "Initializing CPU local objects for CPU %zu\n", SMP::currentCpuId());

    // idle_thread = new IdleThread();
    // debug(A_MULTICORE, "CPU %zu: %s initialized\n", getCpuID(), idle_thread->getName());
    // idle_thread->pinned_to_cpu = getCpuID();
    // Scheduler::instance()->addNewThread(idle_thread);
}

void ArchMulticore::startOtherCPUs()
{
}

void ArchMulticore::stopOtherCpus()
{
}

void ArchMulticore::sendFunctionCallMessage([[maybe_unused]]ArchCpu& cpu, [[maybe_unused]]RemoteFunctionCallMessage* fcall_message)
{
    assert(false && "Not implemented");
}
