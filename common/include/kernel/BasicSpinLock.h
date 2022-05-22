#pragma once
#include "types.h"
#include "EASTL/atomic.h"

class Thread;

class BasicSpinLock
{
public:
    BasicSpinLock();
    BasicSpinLock(const BasicSpinLock&) = delete;

    ~BasicSpinLock() = default;

    void acquire(bool yield = true);
    bool acquireNonBlocking();

    void release();

    Thread* heldBy();

protected:
    eastl::atomic_flag lock_;
    Thread* held_by_ = nullptr;
};
