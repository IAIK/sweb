#pragma once

#include "Device.h"
#include "DeviceDriver.h"
#include "EASTL/string.h"
#include "EASTL/vector.h"
#include "debug.h"

struct DeviceBus : public Device
{
    DeviceBus(const eastl::string bus_name) :
        Device(bus_name)
    {
        debug(DRIVER, "Init '%s' bus\n", deviceName().c_str());
    }

    ~DeviceBus() override = default;

    void addSubDevice(Device& device) override
    {
        debug(DRIVER, "Add '%s' device to '%s' bus\n", device.deviceName().c_str(),
              deviceName().c_str());
        Device::addSubDevice(device);

        if (!device.driver())
        {
            debug(DRIVER, "No driver for '%s' device found\n",
                  device.deviceName().c_str());
        }
    }

    virtual void registerDriver(DeviceDriver& driver)
    {
        debug(DRIVER, "Add driver '%s' to '%s' bus\n", driver.driverName().c_str(),
              deviceName().c_str());

        drivers_.push_back(&driver);
        driver.setParentDevice(*this);
        driver.doDeviceDetection();
    }

private:
    eastl::vector<DeviceDriver*> drivers_;
};

DeviceBus& deviceTreeRoot();
