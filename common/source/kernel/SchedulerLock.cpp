#include "SchedulerLock.h"
#include "debug.h"
#include "Thread.h"
#include "ArchMulticore.h"
#include "Scheduler.h"
#include "ArchCommon.h"

#define INDICATOR_SCHED_LOCK_FREE ' '
#define INDICATOR_SCHED_LOCK_BLOCKED '#'
#define INDICATOR_SCHED_LOCK_HOLDING '-'

SchedulerLock::SchedulerLock() :
    CpuExclusiveLock("Scheduler lock")
{
}

void SchedulerLock::acquire([[maybe_unused]]pointer called_by)
{
    size_t cpu_id = ArchMulticore::getCpuID();
    ((char*)ArchCommon::getFBPtr())[80*2 + cpu_id*2] = INDICATOR_SCHED_LOCK_BLOCKED;

    CpuExclusiveLock::acquire(called_by);

    scheduling_locked_by_ = currentThread;

    debug(SCHEDULER_LOCK, "locked by %s (%p) on CPU %zu\n", (currentThread ? currentThread->getName() : "(nil)"), currentThread, ArchMulticore::getCpuID());

    ((char*)ArchCommon::getFBPtr())[80*2 + cpu_id*2] = INDICATOR_SCHED_LOCK_HOLDING;
}

void SchedulerLock::release([[maybe_unused]]pointer called_by)
{
    size_t cpu_id = ArchMulticore::getCpuID();
    if(currentThread)
    {
        debug(SCHEDULER_LOCK, "unlocked by %s (%p) on CPU %zu\n", currentThread->getName(), currentThread, cpu_id);
    }
    scheduling_locked_by_ = nullptr;

    ((char*)ArchCommon::getFBPtr())[80*2 + cpu_id*2] = INDICATOR_SCHED_LOCK_FREE;

    CpuExclusiveLock::release(called_by);
}
