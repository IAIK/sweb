#include "Mutex.h"
#include "MutexLock.h"
#include "backtrace.h"

MutexLock::MutexLock(Mutex &m, bool b, pointer called_by) :
    mutex_(m), use_mutex_(b)
{
    if (likely(use_mutex_))
    {
        mutex_.acquire(called_by);
    }
}

MutexLock::~MutexLock()
{
    if (likely(use_mutex_))
    {
        mutex_.release(getCalledBefore(1));
        use_mutex_ = false;
    }
}
