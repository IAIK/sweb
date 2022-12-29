#pragma once

#include "Device.h"
#include "DeviceDriver.h"
#include "EASTL/string.h"
#include "EASTL/vector.h"
#include "debug.h"

template<typename DeviceDescriptionType_ = void>
class DeviceBus : public Device
{
    using device_description_type = DeviceDescriptionType_;
private:

    class BusDeviceDriver : public virtual DeviceDriver
    {
    public:
        ~BusDeviceDriver() override = default;
        virtual bool probe(const device_description_type&) = 0;
    };

public:

    // Directly use DeviceDriver instead of BusDeviceDriver<void> (which does not work)
    using bus_device_driver_type = eastl::
        conditional_t<!eastl::is_same_v<device_description_type, void>,
                    BusDeviceDriver,
                    DeviceDriver>;

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
            debug(DRIVER, "Added device '%s' without driver\n",
                  device.deviceName().c_str());
        }
    }

    void registerDriver(bus_device_driver_type& driver)
    {
        debug(DRIVER, "Add driver '%s' to '%s' bus\n", driver.driverName().c_str(),
              deviceName().c_str());

        drivers_.push_back(&driver);
        driver.setParentDevice(*this);
        driver.doDeviceDetection();
    }

    const eastl::vector<bus_device_driver_type*>& drivers() { return drivers_; }

    bool probeDrivers(auto&& device_description)
    {
        debug(DRIVER, "Probe '%s' drivers for device compatibility\n", deviceName().c_str());
        for (auto& d : drivers_)
        {
            if (d->probe(device_description))
            {
                debug(DRIVER, "Found compatible driver '%s' for device description\n", d->driverName().c_str());
                return true;
            }
        }

        debug(DRIVER, "Could not find compatible driver for device description\n");
        return false;
    }

private:
    eastl::vector<bus_device_driver_type*> drivers_;
};

DeviceBus<>& deviceTreeRoot();
