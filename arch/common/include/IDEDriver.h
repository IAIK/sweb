#pragma once

#include <cinttypes>

class BDDriver;

class IDEDriver
{
public:
    IDEDriver();

    ~IDEDriver() = default;

    int32_t processMBR(BDDriver*, uint32_t, uint32_t, const char*);

    uint32_t doDeviceDetection();
};
