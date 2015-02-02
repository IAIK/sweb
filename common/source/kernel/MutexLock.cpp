#include "Mutex.h"
#include "MutexLock.h"

MutexLock::MutexLock(Mutex &m) :
  mutex_(m), use_mutex_(true), debug_info_("")
{
  mutex_.acquire("");
}

MutexLock::MutexLock(Mutex &m, const char* debug_info) :
  mutex_(m), use_mutex_(true), debug_info_(debug_info)
{
  mutex_.acquire(debug_info);
}

MutexLock::MutexLock(Mutex &m, bool b) :
  mutex_(m), use_mutex_(b), debug_info_("")
{
  if (likely (use_mutex_))
    mutex_.acquire("");
}

MutexLock::MutexLock(Mutex &m, bool b, const char* debug_info) :
  mutex_(m), use_mutex_(b), debug_info_(debug_info)
{
  if (likely (use_mutex_))
    mutex_.acquire(debug_info);

}

MutexLock::~MutexLock()
{
  if (likely (use_mutex_))
    mutex_.release(debug_info_);
}
