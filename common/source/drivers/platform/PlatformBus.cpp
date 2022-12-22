#include "PlatformBus.h"

PlatformBus& PlatformBus::instance()
{
    static PlatformBus instance_;
    return instance_;
}

void PlatformBus::initPlatformBus()
{
    deviceTreeRoot().addSubDevice(instance());
}
