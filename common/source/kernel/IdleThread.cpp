#include "IdleThread.h"

#include "Scheduler.h"
#include "VgaColors.h"

#include "ArchCommon.h"
#include "ArchMulticore.h"

#include "assert.h"
#include "debug.h"

IdleThread::IdleThread() :
    Thread(nullptr, "IdleThread", Thread::KERNEL_THREAD)
{
    console_color = CONSOLECOLOR::WHITE;
}

IdleThread::~IdleThread()
{
    assert(false && "Idle thread should not be destroyed");
}

void IdleThread::Run()
{
  uint32 last_ticks = 0;
  uint32 new_ticks = 0;
  while (true)
  {
    new_ticks = Scheduler::instance()->getCpuTimerTicks();
    if (new_ticks == last_ticks)
    {
      last_ticks = new_ticks + 1;
      ArchCommon::idle();
    }
    else
    {
      last_ticks = new_ticks;
      Scheduler::instance()->yield();
    }
  }
}
