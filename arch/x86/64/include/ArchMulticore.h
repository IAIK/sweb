#pragma once

#include "types.h"
#include "CpuLocalScheduler.h"
#include "APIC.h"
#include "uvector.h"
#include "Mutex.h"

class CpuInfo
{
public:
        CpuInfo();

        size_t getCpuID();
        CpuLocalScheduler* getScheduler();

        size_t cpu_id;
        CpuLocalScheduler* scheduler;
private:
};

extern thread_local CpuInfo cpu_info;
extern thread_local CpuLocalScheduler cpu_scheduler;
extern thread_local LocalAPIC lapic;
extern thread_local TSS cpu_tss;

#define AP_STARTUP_PADDR 0x0

class ArchMulticore
{
  public:

    static void initialize();

    static void startOtherCPUs();
    static bool otherCPUsStarted();
    static void stopAllCpus();

    static void setFSBase(uint64 fs_base);
    static void setGSBase(uint64 fs_base);
    static uint64 getFSBase();
    static uint64 getGSBase();
    static uint64 getGSKernelBase();
    static void* getSavedFSBase();

    static void allocCLS(char*& cls, size_t& cls_size);
    static void setCLS(char* cls, size_t cls_size);
    static void initCLS(bool boot_cpu = false);
    static bool CLSinitialized();

    static void setCpuID(size_t id);
    static size_t getCpuID();

    static void initCpu();


    static Mutex cpu_list_lock_;
    static ustl::vector<CpuInfo*> cpu_list_;

    static bool cpus_started_;

  private:
    static void initCpuLocalGDT(SegmentDescriptor* template_gdt);
    static void initCpuLocalTSS(size_t boot_stack_top);
    static void prepareAPStartup(size_t entry_addr);
};
