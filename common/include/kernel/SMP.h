#pragma once

#include <cstddef>
#include "Mutex.h"
#include "EASTL/vector.h"
#include "EASTL/atomic.h"
#include "ArchCpuLocalStorage.h"
#include "ArchMulticore.h"

class ArchCpu;

extern cpu_local ArchCpu current_cpu;

// Information and functions concerning multiple cpus
class SMP
{
public:
    static ArchCpu& currentCpu();
    static size_t currentCpuId();

    static size_t numRunningCpus();

    static void initialize();

    static void addCpuToList(ArchCpu* cpu);

    static Mutex cpu_list_lock_;
    static eastl::vector<ArchCpu*> cpu_list_;
};
