#include "SMP.h"
#include "ArchMulticore.h"
#include "EASTL/atomic.h"
#include "debug.h"

cpu_local ArchCpu current_cpu;
eastl::atomic<size_t> running_cpus;
eastl::vector<ArchCpu*> SMP::cpu_list_;
Mutex SMP::cpu_list_lock_("CPU list lock");

ArchCpu& SMP::currentCpu()
{
    return current_cpu;
}

size_t SMP::currentCpuId()
{
    return (!CpuLocalStorage::ClsInitialized() ? 0 : current_cpu.id());
}

size_t SMP::numRunningCpus()
{
    return running_cpus;
}

void SMP::initialize()
{
    new (&SMP::cpu_list_) eastl::vector<ArchCpu*>;
    new (&SMP::cpu_list_lock_) Mutex("CPU list lock");

    ArchMulticore::initialize();
}

void SMP::addCpuToList(ArchCpu* cpu)
{
    // debug(A_MULTICORE, "Adding ArchCpu %zx to cpu list\n", cpu->getCpuID());
    MutexLock l(SMP::cpu_list_lock_);
    debug(A_MULTICORE, "Locked cpu list, list at %p\n", &SMP::cpu_list_);
    SMP::cpu_list_.push_back(cpu);
    // debug(A_MULTICORE, "Added ArchCpu %zx to cpu list\n", cpu->getCpuID());
}
