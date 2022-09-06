#include "SMP.h"
#include "ArchMulticore.h"
#include "EASTL/atomic.h"
#include "debug.h"

eastl::atomic<size_t> running_cpus;
eastl::vector<CpuInfo*> SMP::cpu_list_;
Mutex SMP::cpu_list_lock_("CPU list lock");

size_t SMP::getCurrentCpuId()
{
    return ArchMulticore::getCurrentCpuId();
}

size_t SMP::numRunningCpus()
{
    return running_cpus;
}

void SMP::initialize()
{
    new (&SMP::cpu_list_) eastl::vector<CpuInfo*>;
    new (&SMP::cpu_list_lock_) Mutex("CPU list lock");

    ArchMulticore::initialize();
}

void SMP::addCpuToList(CpuInfo* cpu)
{
    // debug(A_MULTICORE, "Adding CpuInfo %zx to cpu list\n", cpu->getCpuID());
    MutexLock l(SMP::cpu_list_lock_);
    debug(A_MULTICORE, "Locked cpu list, list at %p\n", &SMP::cpu_list_);
    SMP::cpu_list_.push_back(cpu);
    // debug(A_MULTICORE, "Added CpuInfo %zx to cpu list\n", cpu->getCpuID());
}
