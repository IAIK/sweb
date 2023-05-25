#include "ScopeLock.h"

#include "Mutex.h"
#include "backtrace.h"

ScopeLock::ScopeLock(Mutex& m, bool b, pointer called_by) :
    mutex_(m),
    use_mutex_(b)
{
    if (likely(use_mutex_))
    {
        mutex_.acquire(called_by);
    }
}

ScopeLock::~ScopeLock()
{
    if (likely(use_mutex_))
    {
        mutex_.release(getCalledBefore(1));
        use_mutex_ = false;
    }
}
