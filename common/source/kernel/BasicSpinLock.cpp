#include "BasicSpinLock.h"

#include "Scheduler.h"

#include "ArchCpuLocalStorage.h"
#include "ArchInterrupts.h"
#include "ArchThreads.h"

BasicSpinLock::BasicSpinLock() :
    lock_(0)
{
}

void BasicSpinLock::acquire(bool yield)
{
    Thread* calling_thread = CpuLocalStorage::ClsInitialized() ? currentThread : nullptr;

    while(lock_.test_and_set())
    {
        ArchCommon::spinlockPause();
        //SpinLock: Simplest of Locks, do the next best thing to busy waiting
        if(calling_thread && yield)
        {
            ArchInterrupts::yieldIfIFSet();
        }
    }

    held_by_ = calling_thread;
}

bool BasicSpinLock::acquireNonBlocking()
{
    Thread* calling_thread = CpuLocalStorage::ClsInitialized() ? currentThread : nullptr;

    bool got_lock = !lock_.test_and_set();

    if (got_lock)
    {
        held_by_ = calling_thread;
    }

    return got_lock;
}

void BasicSpinLock::release()
{
    held_by_ = nullptr;
    lock_.clear();
}

Thread* BasicSpinLock::heldBy()
{
    return held_by_;
}

bool BasicSpinLock::isHeldBy(Thread* t)
{
    return heldBy() == t;
}
