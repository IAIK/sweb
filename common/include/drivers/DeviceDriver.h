#pragma once

#include "EASTL/vector.h"
#include "EASTL/string.h"
#include "Device.h"
#include "IrqDomain.h"
#include "ranges.h"
#include "transform.h"
#include "debug.h"

class IrqDomain;

class DeviceDriver
{
public:
    DeviceDriver(const eastl::string& name) :
        driver_name_(name)
    {
        debug(DRIVER, "Driver '%s' created\n", driverName().c_str());
    }

    virtual ~DeviceDriver() = default;

    virtual const eastl::string& driverName()
    {
        return driver_name_;
    }

    virtual eastl::vector<Device*> devices()
    {
        return {};
    }

    virtual void initDriver()
    {
        debug(DRIVER, "Init device driver '%s'\n", driverName().c_str());
    }

    virtual void doDeviceDetection()
    {
        debug(DRIVER, "Driver '%s' device detection\n", driverName().c_str());
    }

    // Allow drivers to initialize cpu local data/devices
    // Do nothing by default
    virtual void cpuLocalInit() { }

    bool isBoundDevice(const Device& device)
    {
        auto bound_devices = devices();
        return eastl::find(bound_devices.begin(), bound_devices.end(), &device) != bound_devices.end();
    }

    Device* parentDevice()
    {
        return parent_device_;
    }

    void setParentDevice(Device& device)
    {
        parent_device_ = &device;
    }

private:
    eastl::string driver_name_;
    Device* parent_device_;
};

template<typename T>
class Driver : public DeviceDriver
{
public:
    using device_type = T;

    Driver(const eastl::string& driver_name) :
        DeviceDriver(driver_name)
    {
    }

    ~Driver() override = default;

    eastl::vector<Device*> devices() override
    {
        auto r = ranges::transform_view(devices_,
                                        [](auto&& x) { return static_cast<Device*>(x); });
        return {r.begin(), r.end()};
    }

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
