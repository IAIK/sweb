#include "Mutex.h"
#include "MutexLock.h"
#include "backtrace.h"

MutexLock::MutexLock(Mutex &m) :
  mutex_(m), use_mutex_(true)
{
  mutex_.acquire(getCalledBefore(1));
}

MutexLock::MutexLock(Mutex &m, bool b) :
  mutex_(m), use_mutex_(b)
{
  if (likely (use_mutex_))
    mutex_.acquire(getCalledBefore(1));
}

MutexLock::~MutexLock()
{
  if (likely (use_mutex_))
    mutex_.release(getCalledBefore(1));
}
