#include "SpinLock.h"
#include "kprintf.h"
#include "ArchThreads.h"
#include "ArchInterrupts.h"
#include "panic.h"
#include "Scheduler.h"
#include "Thread.h"

extern uint32 boot_completed;

SpinLock::SpinLock(const char* name) :
    name_(name), nosleep_mutex_(0), held_by_(0)
{
}

bool SpinLock::acquireNonBlocking(__attribute__((unused)) const char* debug_info)
{
  if (likely(boot_completed))
  {
    if (ArchThreads::testSetLock(nosleep_mutex_, 1))
    {
      return false;
    }
    assert(held_by_ == 0);
    held_by_ = currentThread;
  }
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
    assert(held_by_ == 0);
    held_by_ = currentThread;
  }
}

bool SpinLock::isFree()
{
  if ( unlikely ( ArchInterrupts::testIFSet() && Scheduler::instance()->isSchedulingEnabled() ) )
    kpanict ( ( uint8* ) ( "SpinLock::isFree: ERROR: Should not be used with IF=1 AND enabled Scheduler, use acquire instead\n" ) );

  return (nosleep_mutex_ == 0);
}

void SpinLock::release(const char* debug_info)
{
  if (likely(boot_completed))
  {
    if (held_by_ != currentThread)
    {
      ArchInterrupts::disableInterrupts();
      kprintfd("(ERROR) SpinLock::release with held_by_ (%x) != currentThread (%x) debug_info: %s\n", held_by_, currentThread, debug_info);
      assert(false);
    }
    held_by_ = 0;
    nosleep_mutex_ = 0;
  }
}

void SpinLock::checkInterrupts(const char* method, const char* debug_info)
{
  // it would be nice to assert Scheduler::instance()->isSchedulingEnabled() as well.
  // unfortunately this is difficult because we might want to acquire/release locks
  // while scheduling is disabled
  if ( unlikely ( ArchInterrupts::testIFSet() == false))
  {
    ArchInterrupts::disableInterrupts();
    boot_completed = 0;
    kprintfd("(ERROR) %s: Spinlock %x (%s) with IF=%d (and SchedulingEnabled=%d) ! Now we're dead !!!\n"
             "Maybe you used new/delete in irq/int-Handler context or while Scheduling disabled?\ndebug info:%s\n",
             method, this, name_, ArchInterrupts::testIFSet(), Scheduler::instance()->isSchedulingEnabled(), debug_info);
    currentThread->printBacktrace();
    assert(false);
  }
}
