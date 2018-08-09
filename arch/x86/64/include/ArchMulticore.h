#pragma once

#include "types.h"
#include "CpuLocalScheduler.h"

struct CPULocalStorage
{
  CPULocalStorage* cls_ptr;
  size_t cpu_id;
  SegmentDescriptor gdt[7];
  TSS tss;
  CpuLocalScheduler scheduler;
};

#define AP_STARTUP_PADDR 0x0

class ArchMulticore
{
  public:

    static void initialize();

    static void startOtherCPUs();

    static CPULocalStorage* initCLS();
    static void setCLS(CPULocalStorage* cls);
    static CPULocalStorage* getCLS();
    static bool CLSinitialized();

    static size_t getCpuID();

    static void initCpu();

  private:
    static void prepareAPStartup(size_t entry_addr);
};
