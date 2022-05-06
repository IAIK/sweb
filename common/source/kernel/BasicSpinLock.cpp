#include "BasicSpinLock.h"
#include "Scheduler.h"
#include "ArchThreads.h"
#include "ArchInterrupts.h"

BasicSpinLock::BasicSpinLock() :
    lock_(0)
{
}

void BasicSpinLock::acquire(bool yield)
{
    Thread* calling_thread = CPULocalStorage::CLSinitialized() ? currentThread : nullptr;

    while(lock_.test_and_set())
    {
        //SpinLock: Simplest of Locks, do the next best thing to busy waiting
        if(currentThread && yield)
        {
            ArchInterrupts::yieldIfIFSet();
        }
    }

    held_by_ = calling_thread;
}

bool BasicSpinLock::acquireNonBlocking()
{
    Thread* calling_thread = CPULocalStorage::CLSinitialized() ? currentThread : nullptr;

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
