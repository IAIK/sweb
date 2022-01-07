#include "ArchInterrupts.h"
#include "Scheduler.h"
#include "kprintf.h"
#include "KernelMemoryManager.h"
#include "Thread.h"
#include "types.h"
#include "ArchMulticore.h"

namespace ustl
{
  void checkKMMDeadlock()
  {
    if (ArchMulticore::numRunningCPUs() == 1 && unlikely (ArchInterrupts::testIFSet() == false || Scheduler::instance()->isSchedulingEnabled() == false))
    {
      if (unlikely (KernelMemoryManager::instance()->KMMLockHeldBy() != nullptr))
      {
        system_state = KPANIC;
        kprintfd("(ERROR) checkKMMDeadlock: Using a not resize-safe ustl container method with IF=%d and SchedulingEnabled=%d ! This will fail!!!\n",
                 ArchInterrupts::testIFSet(), Scheduler::instance()->isSchedulingEnabled());
        currentThread->printBacktrace(true);
        assert(false);
      }
    }
  }
}
