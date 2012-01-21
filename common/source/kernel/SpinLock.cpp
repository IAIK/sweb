/**
 * @file SpinLock.cpp
 */

#include "SpinLock.h"
#include "console/kprintf.h"
#include "ArchThreads.h"
#include "ArchInterrupts.h"
#include "panic.h"
#include "Scheduler.h"
#include "Thread.h"

extern uint32 boot_completed;

SpinLock::SpinLock(const char* name) :
  nosleep_mutex_(0), held_by_(0), name_(name)
{
}

bool SpinLock::acquireNonBlocking(const char* debug_info)
{
  if (boot_completed && ArchThreads::testSetLock(nosleep_mutex_, 1))
  {
    return false;
  }
  assert(held_by_ == 0);
  held_by_ = currentThread;
  return true;
}

void SpinLock::acquire(const char* debug_info)
{
  if (likely (boot_completed))
  {
    checkInterrupts("SpinLock::acquire", debug_info);
    while (ArchThreads::testSetLock(nosleep_mutex_, 1))
    {
      //SpinLock: Simplest of Locks, do the next best thing to busy wating
      Scheduler::instance()->yield();
    }
  }
  assert(held_by_ == 0);
  held_by_ = currentThread;
}

bool SpinLock::isFree()
{
  return (nosleep_mutex_ == 0);
}

void SpinLock::release(const char* debug_info)
{
  assert(held_by_ == currentThread);
  held_by_ = 0;
  nosleep_mutex_ = 0;
}

void SpinLock::checkInterrupts(const char* method, const char* debug_info)
{
  if ( unlikely ( ArchInterrupts::testIFSet() == false /*|| !Scheduler::instance()->isSchedulingEnabled()*/))
  {
    kprintfd("(ERROR) %s: Spinlock %x (%s) with IF=%d and SchedulingEnabled=%d ! Now we're dead !!!\n"
             "Maybe you used new/delete in irq/int-Handler context or while Scheduling disabled?\ndebug info:%s\n",
             method, this, name_, ArchInterrupts::testIFSet(), Scheduler::instance()->isSchedulingEnabled(), debug_info);
    currentThread->printBacktrace(false);
    assert(false);
  }
}
