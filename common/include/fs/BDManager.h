#pragma once

#include <cstdint>
#include "EASTL/list.h"
#include "EASTL/atomic.h"

class BDRequest;
class BDVirtualDevice;

class BDManager
{
  public:
    BDManager();
    ~BDManager() = default;

    /**
     * returns singleton instance
     * @return the block device manager instance
     */
    static BDManager& instance();

    /**
     * adds the given device to the manager
     * @param dev the device to add
     */
    void addVirtualDevice(BDVirtualDevice *dev);

    /**
     * returns the device with the given number
     * @param dev_num the device number
     * @return the device
     */
    BDVirtualDevice* getDeviceByNumber(uint32_t dev_num);

    /**
     * returns the device with the given name
     * @param dev_name the device name
     * @return the device
     */
    BDVirtualDevice* getDeviceByName(const char *dev_name);

    /**
     * returns the number of devices in the bd manager
     * @return the number of devices
     */
    uint32_t getNumberOfDevices() const;

    /**
     * adds the given request to the device given in the request
     * @param bdr the request
     */
    void addRequest(BDRequest* bdr);

    /**
     * calls seviceIRQ on the device the irq with the given number is on
     * after that probeIRQ is false
     * @param irq_num the irq number
     */
    void serviceIRQ(uint32_t irq_num);

    /**
     * gets false when the irq is serviced
     */
    eastl::atomic_flag probeIRQ{true};

    eastl::list<BDVirtualDevice*> device_list_;

  protected:
    static BDManager* instance_;
};
