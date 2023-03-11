#pragma once

#include "Mutex.h"
#include "RemoteFunctionCall.h"

#include "ArchCpuLocalStorage.h"
#include "ArchMulticore.h"

#include <cstddef>

#include "EASTL/atomic.h"
#include "EASTL/vector.h"

class ArchCpu;

extern cpu_local ArchCpu current_cpu;


// Information and functions concerning multiple cpus
class SMP
{
public:

    static ArchCpu& currentCpu();
    static size_t currentCpuId();

    static size_t numRunningCpus();

    /**
     * Call a function on all other CPUs
     * Functions run in an interrupt handler and therefore need to be fast and not use any locks
     */
    static void callOnOtherCpus(const RemoteFunctionCallMessage::function_t& func);


    static void initialize();

    static void addCpuToList(ArchCpu* cpu);
    static ArchCpu* cpu(size_t cpu_id);

    static eastl::vector<ArchCpu*>& cpuList();
    static Mutex& cpuListLock();
};
