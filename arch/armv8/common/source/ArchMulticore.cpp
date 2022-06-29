#include "ArchMulticore.h"
#include "EASTL/atomic.h"

eastl::atomic<size_t> running_cpus;
Mutex ArchMulticore::cpu_list_lock_("CPU list lock");
eastl::vector<CpuInfo*> ArchMulticore::cpu_list_;

size_t ArchMulticore::getCpuID()
{
    return 0;
}

size_t ArchMulticore::numRunningCPUs()
{
    return running_cpus;
}

void ArchMulticore::stopOtherCpus()
{
}

bool CPULocalStorage::CLSinitialized()
{
    return true;
}

void ArchMulticore::initialize()
{
    new (&cpu_list_) eastl::vector<CpuInfo*>;
    new (&cpu_list_lock_) Mutex("CPU list lock");

    assert(running_cpus == 0);
    running_cpus = 1;
    // CPULocalStorage::setCLS(CPULocalStorage::allocCLS());
    // ArchMulticore::initCPULocalData(true);
}

void ArchMulticore::startOtherCPUs()
{

}

size_t CpuInfo::getCpuID()
{
    return 0;
}

void CpuInfo::setCpuID([[maybe_unused]]size_t id)
{
}
