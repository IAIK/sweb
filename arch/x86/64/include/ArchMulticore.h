#pragma once

#include "types.h"
#include "CpuLocalScheduler.h"
#include "APIC.h"
#include "uvector.h"
#include "Mutex.h"

class CpuLocalStorage
{
public:
  CpuLocalStorage();

  CpuLocalStorage* init();

  size_t getCpuID();

  CpuLocalStorage* cls_ptr;
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
    static void stopAllCpus();

    static CpuLocalStorage* initCLS();
    static CpuLocalStorage* getCLS();
    static bool CLSinitialized();

    static void setCpuID(size_t id);
    static size_t getCpuID();

    static void initCpu();

    static Mutex cpu_list_lock_;
    static ustl::vector<CpuLocalStorage*> cpu_list_;

  private:
    static void prepareAPStartup(size_t entry_addr);
};
