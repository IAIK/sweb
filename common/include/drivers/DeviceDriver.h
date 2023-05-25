#pragma once

#include "Device.h"
#include "IrqDomain.h"

#include "ranges.h"
#include "transform.h"

#include "EASTL/string.h"
#include "EASTL/vector.h"

#include "debug.h"

class IrqDomain;

// Abstract interface for device drivers
class DeviceDriver
{
public:
    virtual ~DeviceDriver() = default;

    virtual const eastl::string& driverName() = 0;
    virtual void initDriver() = 0;
    virtual void doDeviceDetection() = 0;
    virtual void cpuLocalInit() = 0;
    virtual Device* parentDevice() = 0;
    virtual void setParentDevice(Device& device) = 0;
};

// Provides default implementation for DeviceDriver functions
class BasicDeviceDriver : public virtual DeviceDriver
{
public:
    BasicDeviceDriver(const eastl::string& name) :
        driver_name_(name)
    {
        debug(DRIVER, "Driver '%s' created\n", driverName().c_str());
    }

    ~BasicDeviceDriver() override = default;

    const eastl::string& driverName() override
    {
        return driver_name_;
    }

    void initDriver() override
    {
        debug(DRIVER, "Init device driver '%s'\n", driverName().c_str());
    }

    void doDeviceDetection() override
    {
        debug(DRIVER, "Driver '%s' device detection\n", driverName().c_str());
    }

    // Allow drivers to initialize cpu local data/devices
    // Do nothing by default
    void cpuLocalInit() override { }

    Device* parentDevice() override
    {
        return parent_device_;
    }

    void setParentDevice(Device& device) override
    {
        parent_device_ = &device;
    }

private:
    eastl::string driver_name_;
    Device* parent_device_;
};

// Additional driver functions for concrete device types
template<typename T>
class Driver : public virtual DeviceDriver
{
public:
    using device_type = T;

    Driver() = default;
    ~Driver() override = default;

protected:
    void bindDevice(T& device)
    {
        device.setDriver(*this);
        devices_.push_back(&device);
        if (parentDevice() && !device.parent())
        {
            parentDevice()->addSubDevice(device);
        }
    }

    eastl::vector<T*> devices_;

private:
};
