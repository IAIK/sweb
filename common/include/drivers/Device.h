#pragma once

#include "EASTL/string.h"
#include "EASTL/vector.h"
#include "debug.h"
#include "ranges.h"

class DeviceDriver;

struct Device
{
    Device(const eastl::string& device_type, Device* parent = nullptr, DeviceDriver* driver = nullptr) :
        parent_(parent),
        driver_(driver),
        device_name_(device_type)
    {
        debug(DRIVER, "Create device '%s'\n", deviceName().c_str());
        if (parent)
        {
            parent->addSubDevice(*this);
        }
    }
    virtual ~Device() = default;


    Device& setParent(Device& parent)
    {
        parent_ = &parent;
        return *this;
    }

    Device* parent()
    {
        return parent_;
    }

    DeviceDriver* driver()
    {
        return driver_;
    }

    Device& setDriver(DeviceDriver& driver)
    {
        driver_ = &driver;
        return *this;
    }

    virtual const eastl::string& deviceName()
    {
        return device_name_;
    }

    void setDeviceName(const eastl::string& name)
    {
        device_name_ = name;
    }

    auto subdevices()
    {
        return ranges::subrange(subdevices_);
    }

    virtual void addSubDevice(Device& subdevice)
    {
        debug(DRIVER, "Adding sub device %s to %s\n", subdevice.deviceName().c_str(),
              deviceName().c_str());
        if (subdevice.parent())
        {
            debug(DRIVER, "ERROR: subdevice added to %s already has parent %s\n", deviceName().c_str(), subdevice.parent()->deviceName().c_str());
        }
        assert(!subdevice.parent() || subdevice.parent() == this);
        subdevice.setParent(*this);
        subdevices_.push_back(&subdevice);
    }

    void printSubDevices(int level = 0)
    {
        kprintfd("%*s- %s\n", level*2, "", deviceName().c_str());
        for (auto&& x : subdevices())
        {
            x->printSubDevices(level + 1);
        }
    }


protected:

    Device* parent_; // Usually a bus or controller, NULL for top level devices
    DeviceDriver* driver_;
    eastl::string device_name_;
    eastl::vector<Device*> subdevices_;
};
