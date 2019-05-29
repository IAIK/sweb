#pragma once

#include "types.h"
#include "APIC.h"
#include "uvector.h"
#include "Mutex.h"
#include "SegmentUtils.h"
#include "uatomic.h"
#include "IdleThread.h"


struct TLBShootdownRequest;

class CpuInfo
{
public:
        CpuInfo();

        size_t getCpuID();
        void setCpuID(size_t id);

        LocalAPIC lapic;

        size_t cpu_id;

        ustl::atomic<TLBShootdownRequest*> tlb_shootdown_list;
private:
};

#define CPU_STACK_SIZE 4*PAGE_SIZE

extern thread_local CpuInfo cpu_info;
extern thread_local TSS cpu_tss;

extern thread_local IdleThread idle_thread;

#define AP_STARTUP_PADDR 0x0

class ArchMulticore
{
  public:

    static void initialize();

    static void startOtherCPUs();
    static size_t numRunningCPUs();
    static void stopAllCpus();



    static void allocCLS(char*& cls, size_t& cls_size);
    static void setCLS(char* cls, size_t cls_size);
    static void initCLS(bool boot_cpu = false);
    static bool CLSinitialized();

    static void setCpuID(size_t id);
    static size_t getCpuID();

    static void initCpu();

    static char* cpuStackTop();

    static Mutex cpu_list_lock_;
    static ustl::vector<CpuInfo*> cpu_list_;

  private:
    static void initCpuLocalGDT(GDT& template_gdt);
    static void initCpuLocalTSS(size_t boot_stack_top);
    static void prepareAPStartup(size_t entry_addr);

    static void waitForSystemStart();
};
