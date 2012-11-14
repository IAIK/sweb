#include "ArchInterrupts.h"
#include "Scheduler.h"
#include "kprintf.h"
#include "KernelMemoryManager.h"
#include "Thread.h"
#include "types.h"

extern uint32 boot_completed;

namespace ustl
{
  void checkKMMDeadlock()
  {
    if (unlikely (ArchInterrupts::testIFSet() == false || Scheduler::instance()->isSchedulingEnabled() == false))
    {
      if (unlikely (KernelMemoryManager::instance()->KMMLockHeldBy() != 0))
      {
        boot_completed = 0;
        kprintfd("(ERROR) checkKMMDeadlock: Using a not resize-safe ustl container method with IF=%d and SchedulingEnabled=%d ! This will fail!!!\n",
                 ArchInterrupts::testIFSet(), Scheduler::instance()->isSchedulingEnabled());
        currentThread->printBacktrace(true);
        prenew_assert(false);
      }
    }
  }
}
