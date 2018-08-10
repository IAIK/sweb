#pragma once

#include "types.h"
#include "CpuLocalScheduler.h"
#include "APIC.h"

class CPULocalStorage
{
public:
  CPULocalStorage();

  CPULocalStorage* init();

  CPULocalStorage* cls_ptr;
  size_t cpu_id;
  SegmentDescriptor gdt[7];
  TSS tss;
  CpuLocalScheduler scheduler;
  LocalAPIC apic;
private:
};

#define AP_STARTUP_PADDR 0x0

class ArchMulticore
{
  public:

    static void initialize();

    static void startOtherCPUs();

    static CPULocalStorage* initCLS();
    static CPULocalStorage* getCLS();
    static bool CLSinitialized();

    static void setCpuID(size_t id);
    static size_t getCpuID();

    static void initCpu();

  private:
    static void prepareAPStartup(size_t entry_addr);
};
