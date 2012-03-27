#include "ArchInterrupts.h"
#include "Scheduler.h"
#include "KernelMemoryManager.h"

namespace ustl {
  void checkKMMDeadlock()
  {
    if (ArchInterrupts::testIFSet() == false || Scheduler::instance()->isSchedulingEnabled() == false)
        assert(KernelMemoryManager::instance()->KMMLockHeldBy() == 0);
  }
}
