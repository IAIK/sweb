#include "ArchMulticore.h"
#include "EASTL/atomic.h"
#include "SMP.h"
#include "debug.h"

extern eastl::atomic<size_t> running_cpus;
cpu_local size_t cpu_id;
cpu_local CpuInfo cpu_info;

size_t ArchMulticore::getCurrentCpuId()
{
    uint64_t mpidr_el1 = 0;
    asm("MRS %[mpidr_el1], MPIDR_EL1\n"
        :[mpidr_el1]"=g"(mpidr_el1));
    return mpidr_el1 & 0xFF;
}

size_t ArchMulticore::numRunningCPUs()
{
    return running_cpus;
}

void ArchMulticore::stopOtherCpus()
{
}

extern char cls_start;
extern char cls_end;
extern char tbss_start;
extern char tbss_end;
extern char tdata_start;
extern char tdata_end;


void* CPULocalStorage::getClsBase()
{
    void* cls = 0;
    asm("MRS %[cls], TPIDR_EL1\n"
        :[cls]"=g"(cls));
    return cls;
}

bool CPULocalStorage::CLSinitialized()
{
    return getClsBase();
}

size_t CPULocalStorage::getCLSSize()
{
    return &cls_end - &cls_start;
}

char* CPULocalStorage::allocCLS()
{
  debug(A_MULTICORE, "Allocating CPU local storage\n");

  size_t cls_size = getCLSSize();
  size_t tbss_size = &tbss_end - &tbss_start;
  size_t tdata_size = &tdata_end - &tdata_start;
  debug(A_MULTICORE, "cls_base: [%p, %p), size: %zx\n", &cls_start, &cls_end, cls_size);
  debug(A_MULTICORE, "tbss: [%p, %p), size: %zx\n", &tbss_start, &tbss_end, tbss_size);
  debug(A_MULTICORE, "tdata: [%p, %p), size: %zx\n", &tdata_start, &tdata_end, tdata_size);

  char* cls_base = new char[cls_size + sizeof(void*)]{};
  debug(A_MULTICORE, "Allocated new cls_base at [%p, %p)\n", cls_base, cls_base + cls_size + sizeof(void*));

  debug(A_MULTICORE, "Initializing tdata at [%p, %p) and tbss at [%p, %p)\n",
        cls_base + (&tdata_start - &cls_start), cls_base + (&tdata_start - &cls_start) + tdata_size,
        cls_base + (&tbss_start - &cls_start), cls_base + (&tbss_start - &cls_start) + tbss_size);
  memcpy(cls_base + (&tdata_start - &cls_start), &tdata_start, tdata_size);

  return cls_base;
}

void CPULocalStorage::setCLS(char* cls)
{
    debug(A_MULTICORE, "Set CLS: %p\n", cls);
    asm("MSR TPIDR_EL1, %[cls]\n"
        ::[cls]"g"(cls));
}

void ArchMulticore::initialize()
{
    assert(running_cpus == 0);
    running_cpus = 1;
    CPULocalStorage::setCLS(CPULocalStorage::allocCLS());
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

void ArchMulticore::startOtherCPUs()
{

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
