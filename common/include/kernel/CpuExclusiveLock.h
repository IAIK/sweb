#pragma once

#include "types.h"
#include "EASTL/atomic.h"

class CpuExclusiveLock
{
public:
    CpuExclusiveLock(const char* name);

    CpuExclusiveLock(const CpuExclusiveLock&) = delete;

    virtual void acquire(pointer called_by = 0);
    virtual void release(pointer called_by = 0);

    bool isHeldBy(size_t cpu_id) const;
    size_t heldBy() const;

    const char* getName() const;

protected:
    eastl::atomic<size_t> held_by_cpu_;

    const char* name_;
};
