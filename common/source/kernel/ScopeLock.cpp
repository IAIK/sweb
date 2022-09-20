#include "Mutex.h"
#include "ScopeLock.h"
#include "backtrace.h"

ScopeLock::ScopeLock(Mutex &m) :
  mutex_(m), use_mutex_(true)
{
  mutex_.acquire(getCalledBefore(1));
}

ScopeLock::ScopeLock(Mutex &m, bool b) :
  mutex_(m), use_mutex_(b)
{
  if (likely (use_mutex_))
    mutex_.acquire(getCalledBefore(1));
}

ScopeLock::~ScopeLock()
{
  if (likely (use_mutex_))
    mutex_.release(getCalledBefore(1));
}
