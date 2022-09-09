#pragma once

#include "types.h"
#include "APIC.h"
#include "EASTL/vector.h"
#include "Mutex.h"
#include "SegmentUtils.h"
#include "EASTL/atomic.h"
#include "IdleThread.h"
#include "paging-definitions.h"
#include "ArchCpuLocalStorage.h"
#include "Cpu.h"


#define CPU_STACK_SIZE 4*PAGE_SIZE

class Allocator;


struct TLBShootdownRequest
{
        size_t addr;
        eastl::atomic<size_t> ack;
        size_t target;
        eastl::atomic<TLBShootdownRequest*> next;
        size_t request_id;
        size_t orig_cpu;
};

class ArchCpu : public Cpu
{
public:
        ArchCpu();

        LocalAPIC* lapic;

        eastl::atomic<TLBShootdownRequest*> tlb_shootdown_list;
private:
};


extern cpu_local LocalAPIC cpu_lapic;
extern cpu_local char cpu_stack[CPU_STACK_SIZE];
extern cpu_local TSS cpu_tss;
extern cpu_local IdleThread* idle_thread;

#define AP_STARTUP_PADDR 0x0

class ArchMulticore
{
  public:
    static void initialize();

    static void reservePages(Allocator& alloc);

    static void startOtherCPUs();
    static void stopAllCpus();
    static void stopOtherCpus();

    static void initApplicationProcessorCpu();
    static void initCpuLocalData(bool boot_cpu = false);


    static char* cpuStackTop();

  private:
    static void initCpuLocalGDT(GDT& template_gdt);
    static void initCpuLocalTSS(size_t boot_stack_top);
    static void prepareAPStartup(size_t entry_addr);

    [[noreturn]] static void waitForSystemStart();
};
