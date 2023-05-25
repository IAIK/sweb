#include "SchedulerLock.h"

#include "Scheduler.h"
#include "Thread.h"

#include "ArchCommon.h"
#include "ArchMulticore.h"

#include "debug.h"

enum SCHED_LOCK_INDICATOR : char
{
    FREE = ' ',
    BLOCKED = '#',
    HOLDING = '-',
};

SchedulerLock::SchedulerLock() :
    CpuExclusiveLock("Scheduler lock")
{
}

void SchedulerLock::acquire([[maybe_unused]]pointer called_by)
{
    size_t cpu_id = SMP::currentCpuId();
    ((char*)ArchCommon::getFBPtr())[80*2 + cpu_id*2] = SCHED_LOCK_INDICATOR::BLOCKED;

    CpuExclusiveLock::acquire(called_by);

    scheduling_locked_by_ = currentThread;

    debug(SCHEDULER_LOCK, "locked by %s (%p) on CPU %zu\n", (currentThread ? currentThread->getName() : "(nil)"), currentThread, SMP::currentCpuId());

    ((char*)ArchCommon::getFBPtr())[80*2 + cpu_id*2] = SCHED_LOCK_INDICATOR::HOLDING;
}

void SchedulerLock::release([[maybe_unused]]pointer called_by)
{
    size_t cpu_id = SMP::currentCpuId();
    if(currentThread)
    {
        debug(SCHEDULER_LOCK, "unlocked by %s (%p) on CPU %zu\n", currentThread->getName(), currentThread, cpu_id);
    }
    scheduling_locked_by_ = nullptr;

    ((char*)ArchCommon::getFBPtr())[80*2 + cpu_id*2] = SCHED_LOCK_INDICATOR::FREE;

    CpuExclusiveLock::release(called_by);
}
