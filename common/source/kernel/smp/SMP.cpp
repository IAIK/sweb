#include "SMP.h"
#include "ArchMulticore.h"
#include "EASTL/atomic.h"
#include "debug.h"

cpu_local ArchCpu current_cpu;
eastl::atomic<size_t> running_cpus;
Mutex SMP::cpu_list_lock_("CPU list lock");

extern cpu_local Thread* currentThread;

ArchCpu& SMP::currentCpu()
{
    return current_cpu;
}

size_t SMP::currentCpuId()
{
    return (!CpuLocalStorage::ClsInitialized() ? 0 : currentCpu().id());
}

size_t SMP::numRunningCpus()
{
    return running_cpus;
}

void SMP::initialize()
{
    new (&SMP::cpu_list_lock_) Mutex("CPU list lock");

    ArchMulticore::initialize();
}

void SMP::addCpuToList(ArchCpu* cpu)
{
    ScopeLock l(SMP::cpu_list_lock_);
    SMP::cpuList().push_back(cpu);
}

eastl::vector<ArchCpu*>&  SMP::cpuList()
{
    static eastl::vector<ArchCpu*> cpu_list_;
    return cpu_list_;
}

ArchCpu* SMP::cpu(size_t cpu_id)
{
    ScopeLock l(SMP::cpu_list_lock_);
    for (auto c : SMP::cpuList())
    {
        if (c->id() == cpu_id)
            return c;
    }
    return nullptr;
}

void SMP::callOnOtherCpus(const RemoteFunctionCallMessage::function_t& func)
{
    assert(func);
    debug(A_MULTICORE, "Calling function on other cpus\n");

    // Prepare
    RemoteFunctionCallMessage funcdata[SMP::cpuList().size()]{};

    auto orig_cpu = SMP::currentCpuId();

    for (auto& fd : funcdata)
    {
        fd.func = func;
        fd.orig_cpu = orig_cpu;
        fd.target_cpu = -1;
        fd.done = false;
        fd.received = false;
    }

    // Send
    for (auto* cpu : SMP::cpuList())
    {
        auto id = cpu->id();
        funcdata[id].target_cpu = id;

        if (id != orig_cpu)
        {
            ArchMulticore::sendFunctionCallMessage(*cpu, &funcdata[id]);
        }
    }

    // Wait for ack
    for (auto& fd : funcdata)
    {
        assert(fd.target_cpu != (size_t)-1);
        if (fd.target_cpu == orig_cpu)
            continue;

        if (!fd.done.load(eastl::memory_order_acquire))
        {
            if (A_MULTICORE & OUTPUT_ADVANCED)
                debug(A_MULTICORE, "Waiting for ack from CPU %zu for request from CPU %zu, received: %u\n",
                      fd.target_cpu, orig_cpu, fd.received.load(eastl::memory_order_acquire));
        }

        while(!fd.done.load(eastl::memory_order_acquire));
    }
}
