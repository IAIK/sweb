#pragma once

#include <cinttypes>
#include "DeviceDriver.h"

class BDDriver;

class IDEDriver : public DeviceDriver
{
public:
    IDEDriver();

    ~IDEDriver() override = default;

    static IDEDriver& instance();

    int32_t processMBR(BDDriver*, uint32_t, uint32_t, const char*);

    void doDeviceDetection() override;
};
