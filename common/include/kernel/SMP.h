#pragma once

#include <cstddef>
#include "Mutex.h"
#include "EASTL/vector.h"
#include "EASTL/atomic.h"

class CpuInfo;

class SMP
{
public:
    static size_t getCurrentCpuId();

    static size_t numRunningCpus();

    static void initialize();

    static void addCpuToList(CpuInfo* cpu);

    static Mutex cpu_list_lock_;
    static eastl::vector<CpuInfo*> cpu_list_;
};
