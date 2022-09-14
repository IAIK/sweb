#include "TimerTickHandler.h"
#include "Scheduler.h"

// Architecture independent timer tick handler
void TimerTickHandler::handleTimerTick()
{
    ArchCommon::drawHeartBeat();
    Scheduler::instance()->incCpuTimerTicks();
    Scheduler::instance()->schedule();
}
