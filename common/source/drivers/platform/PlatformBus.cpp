#include "PlatformBus.h"

PlatformBus& PlatformBus::instance()
{
    static PlatformBus instance_;
    return instance_;
}

void PlatformBus::initPlatformBus()
{
    DeviceBus::root().addSubDevice(instance());
}
