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
    while(ArchThreads::testSetLock(lock_, (unsigned int)1))
    {
        //SpinLock: Simplest of Locks, do the next best thing to busy waiting
        if(currentThread && yield)
        {
            ArchInterrupts::yieldIfIFSet();
        }
    }
}

bool BasicSpinLock::acquireNonBlocking()
{
    return ArchThreads::testSetLock(lock_, (unsigned int)1) == 0;
}

void BasicSpinLock::release()
{
    ArchThreads::atomic_set(lock_, 0);
}
