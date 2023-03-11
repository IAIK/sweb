#include "TimerTickHandler.h"

#include "Scheduler.h"

// Architecture independent timer tick handler
void TimerTickHandler::handleTimerTick()
{
    ArchCommon::drawHeartBeat();
    Scheduler::instance()->incCpuTimerTicks();
    debugAdvanced(A_INTERRUPTS, "Timer tick %zu\n", Scheduler::instance()->getCpuTimerTicks());
    Scheduler::instance()->schedule();
}
