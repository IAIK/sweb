#include "SMP.h"
#include "ArchMulticore.h"
#include "EASTL/atomic.h"
#include "debug.h"

cpu_local ArchCpu current_cpu;
eastl::atomic<size_t> running_cpus;
eastl::vector<ArchCpu*> SMP::cpu_list_;
Mutex SMP::cpu_list_lock_("CPU list lock");

ArchCpu& SMP::currentCpu()
{
    return current_cpu;
}

size_t SMP::currentCpuId()
{
    return (!CpuLocalStorage::ClsInitialized() ? 0 : current_cpu.id());
}

size_t SMP::numRunningCpus()
{
    return running_cpus;
}

void SMP::initialize()
{
    new (&SMP::cpu_list_) eastl::vector<ArchCpu*>;
    new (&SMP::cpu_list_lock_) Mutex("CPU list lock");

    ArchMulticore::initialize();
}

void SMP::addCpuToList(ArchCpu* cpu)
{
    // debug(A_MULTICORE, "Adding ArchCpu %zx to cpu list\n", cpu->getCpuID());
    MutexLock l(SMP::cpu_list_lock_);
    debug(A_MULTICORE, "Locked cpu list, list at %p\n", &SMP::cpu_list_);
    SMP::cpu_list_.push_back(cpu);
    // debug(A_MULTICORE, "Added ArchCpu %zx to cpu list\n", cpu->getCpuID());
}

void SMP::callOnOtherCpus(const RemoteFunctionCallMessage::function_t& func)
{
    assert(func);
    debug(A_MULTICORE, "Calling function on other cpus\n");

    // Prepare
    RemoteFunctionCallMessage funcdata[SMP::cpu_list_.size()]{};

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
    for(auto* cpu : SMP::cpu_list_)
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