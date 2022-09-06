#include "CpuExclusiveLock.h"
#include "ArchInterrupts.h"
#include "ArchMulticore.h"
#include "ArchCommon.h"
#include "SMP.h"
#include "kprintf.h"
#include "assert.h"

CpuExclusiveLock::CpuExclusiveLock(const char* name) :
    held_by_cpu_(-1),
    name_(name)
{
}

// Thread must not be re-scheduled to other cpus while holding CPU exclusive locks!
// i.e. disable scheduling or pin thread to cpu
void CpuExclusiveLock::acquire([[maybe_unused]]pointer called_by)
{
    {
        // CPU must not change between reading CPU ID and setting the lock value
        WithInterrupts intr(false);

        size_t expected = -1;
        size_t cpu_id = SMP::getCurrentCpuId();

        if(held_by_cpu_.load() == cpu_id)
        {
            kprintfd("ERROR: cpu exclusive lock %s already locked by cpu %zd\n", getName(), cpu_id);
            assert(false && "CPU exclusive lock already held by current cpu");
        }

        while(!held_by_cpu_.compare_exchange_weak(expected, cpu_id))
        {
            expected = -1;
            ArchCommon::spinlockPause();
        }
    }
}

void CpuExclusiveLock::release([[maybe_unused]]pointer called_by)
{
    size_t cpu_id = SMP::getCurrentCpuId();

    size_t was_locked_by = held_by_cpu_.exchange(-1);
    if(was_locked_by != cpu_id)
    {
        kprintfd("ERROR: CPU exclusive lock %s unlocked by CPU %zd, but was locked by CPU %zd\n", getName(), cpu_id, was_locked_by);
    }
    assert(was_locked_by == cpu_id);
}


const char* CpuExclusiveLock::getName() const
{
    return name_;
}

bool CpuExclusiveLock::isHeldBy(size_t cpu_id) const
{
    return held_by_cpu_ == cpu_id;
}

size_t CpuExclusiveLock::heldBy() const
{
    return held_by_cpu_;
}
