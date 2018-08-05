#pragma once

#include "types.h"

struct CoreLocalStorage
{
  CoreLocalStorage* cls_ptr;
  size_t core_id;
  SegmentDescriptor gdt[7];
  TSS tss;
};

class ArchMulticore
{
  public:

    static void initialize();

    static void startOtherCPUs();

    static CoreLocalStorage* initCLS();
    static void setCLS(CoreLocalStorage* cls);
    static CoreLocalStorage* getCLS();

    static size_t getCoreID();

    static void initCore();

  private:
};
