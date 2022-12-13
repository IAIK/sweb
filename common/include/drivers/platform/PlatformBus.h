#pragma once

#include "Device.h"
#include "DeviceBus.h"

struct PlatformBus : public DeviceBus
{
    PlatformBus() :
        DeviceBus("Platform")
    {
    }

    ~PlatformBus() override = default;

    static PlatformBus& instance();

    static void initPlatformBus();
};
