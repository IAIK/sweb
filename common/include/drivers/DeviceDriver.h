#pragma once

#include "EASTL/vector.h"
#include "EASTL/string.h"
#include "Device.h"
#include "IrqDomain.h"
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

    virtual void cpuLocalInit()
    {
        // Allow drivers to initialize cpu local data/devices
        // Do nothing by default
    }

    // virtual bool probe(Device& device)
    // {
    //     debug(DRIVER, "Probe device driver %s for device %s\n", driverName().c_str(), device.id().c_str());
    //     return false;
    // }

    // virtual void init(Device& device)
    // {
    //     debug(DRIVER, "Device driver '%s' init device '%s'\n", driverName().c_str(), device.id().c_str());
    // }

    // virtual void bindDevice(Device& device)
    // {
    //     debug(DRIVER, "Bind device '%s' to driver '%s'\n", device.id().c_str(), driverName().c_str());
    //     device.setDriver(this);
    //     devices_.push_back(&device);
    // }

    // void bindAndInitDevice(Device& device)
    // {
    //     bindDevice(device);
    //     assert(device.driver() == this);
    //     init(device);
    // }

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

    // virtual IrqDomain* getDeviceIrqDomain(Device& device)
    // {
    //     assert(isBoundDevice(device));
    //     assert(device.interrupt_controller);
    //     return nullptr;
    // }

    // virtual void registerIrq(Device& source_device, Device& irq_target, size_t irqnum, [[maybe_unused]]void (*handler) (void))
    // {
    //     debug(DRIVER, "Register IRQ %zu from device %s -> %s, handler: %p\n", irqnum, source_device.id().c_str(), irq_target.id().c_str(), handler);
    //     // TODO: need to create mapping from source irq domain to target irq domain
    //     // this driver is responsible for creating and selecting the target irq domain, but the source irq domain needs to be provided by the driver of the irq source (passed as argument?)
    // }

    // virtual bool registerIrq(IrqDomain& source_domain, irqnum_t source_irq, Device& irq_target, irqnum_t target_irq, [[maybe_unused]]void (*handler) (void))
    // {
    //     debug(DRIVER, "Register IRQ %s:%zu -> %s:%zu, handler: %p\n", source_domain.name().c_str(), source_irq, irq_target.id().c_str(), target_irq, handler);
    //     return false;
    // }

    // eastl::vector<Device*> devices_;
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

    virtual ~Driver() = default;

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
