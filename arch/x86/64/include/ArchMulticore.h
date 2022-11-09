#pragma once

#include "types.h"
#include "APIC.h"
#include "X2Apic.h"
#include "EASTL/vector.h"
#include "Mutex.h"
#include "SegmentUtils.h"
#include "EASTL/atomic.h"
#include "IdleThread.h"
#include "paging-definitions.h"
#include "ArchCpuLocalStorage.h"
#include "SMP.h"
#include "Cpu.h"
#include "AtomicMpScQueue.h"
#include "RemoteFunctionCall.h"

#define CPU_STACK_SIZE 4*PAGE_SIZE

#define STOP_INT_VECTOR 90
#define MESSAGE_INT_VECTOR 101


class ArchCpu : public Cpu
{
public:
        ArchCpu();

        Apic* lapic;
private:
};


extern cpu_local Apic* cpu_lapic;
extern cpu_local char cpu_stack[CPU_STACK_SIZE];
extern cpu_local TSS cpu_tss;
extern cpu_local IdleThread* idle_thread;

#define AP_STARTUP_PADDR 0x0

class Allocator;

class ArchMulticore
{
  public:
    static void initialize();

    static void reservePages(Allocator& alloc);

    static void startOtherCPUs();
    static void stopAllCpus();
    static void stopOtherCpus();

    static void sendFunctionCallMessage(ArchCpu& cpu, RemoteFunctionCallMessage* fcall_message);
    static void notifyMessageAvailable(ArchCpu& cpu);

    static void initCpu();
    static void initCpuLocalData(bool boot_cpu = false);

    static char* cpuStackTop();

  private:
    static void initCpuLocalGDT(GDT& template_gdt);
    static void initCpuLocalTSS(size_t boot_stack_top);
    static void prepareAPStartup(size_t entry_addr);

    [[noreturn]] static void waitForSystemStart();
};
