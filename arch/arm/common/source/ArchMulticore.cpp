#include "ArchMulticore.h"
#include "SMP.h"
#include "debug.h"

cpu_local size_t cpu_id;
cpu_local CpuInfo cpu_info;

void ArchMulticore::initialize()
{
    // CPULocalStorage::setCLS(CPULocalStorage::allocCLS());
    ArchMulticore::initCpuLocalData(true);
}

void ArchMulticore::initCpuLocalData([[maybe_unused]]bool boot_cpu)
{

    // The constructor of objects declared as cpu_local will be called automatically the first time the cpu_local object is used. Other cpu_local objects _may or may not_ also be initialized at the same time.
    new (&cpu_info) CpuInfo();
    debug(A_MULTICORE, "Initializing CPU local objects for CPU %zu\n", cpu_info.getCpuID());

    // idle_thread = new IdleThread();
    // debug(A_MULTICORE, "CPU %zu: %s initialized\n", getCpuID(), idle_thread->getName());
    // idle_thread->pinned_to_cpu = getCpuID();
    // Scheduler::instance()->addNewThread(idle_thread);
}

size_t ArchMulticore::getCurrentCpuId()
{
    return 0;
}

size_t ArchMulticore::numRunningCPUs()
{
    return 1;
}

void ArchMulticore::startOtherCPUs()
{
}

void ArchMulticore::stopOtherCpus()
{
}

bool CPULocalStorage::CLSinitialized()
{
    return true;
}



CpuInfo::CpuInfo() :
    cpu_id_(&cpu_id)
{
    setCpuID(ArchMulticore::getCurrentCpuId());
    debug(A_MULTICORE, "Initializing CpuInfo %zx\n", getCpuID());
    SMP::addCpuToList(this);
}

size_t CpuInfo::getCpuID()
{
    return *cpu_id_;
}

void CpuInfo::setCpuID(size_t id)
{
    *cpu_id_ = id;
}
