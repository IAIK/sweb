#include "BDManager.h"
#include "BDDriver.h"
#include "BDRequest.h"
#include "BDVirtualDevice.h"
#include "PlatformBus.h"
#include "kprintf.h"
#include "kstring.h"
#include "debug.h"

BDManager& BDManager::instance()
{
    static BDManager instance_;
    return instance_;
}


BDManager::BDManager() :
    probeIRQ(false)
{
}

void BDManager::addRequest(BDRequest* bdr)
{
  if (bdr->getDevID() < getNumberOfDevices())
    getDeviceByNumber(bdr->getDevID())->addRequest(bdr);
  else
    bdr->setStatus(BDRequest::BD_RESULT::BD_ERROR);
}

void BDManager::addVirtualDevice(BDVirtualDevice* dev)
{
  debug(BD_MANAGER, "addVirtualDevice: Adding device\n");
  dev->setDeviceNumber(device_list_.size());
  device_list_.push_back(dev);
  debug(BD_MANAGER, "addVirtualDevice: Device added\n");
}

void BDManager::serviceIRQ(uint32 irq_num)
{
  debug(BD_MANAGER, "serviceIRQ: Servicing IRQ\n");
  probeIRQ = false;

  for (BDVirtualDevice* dev : device_list_)
    if (dev->getDriver()->irq == irq_num)
    {
      dev->getDriver()->serviceIRQ();
      return;
    }

  debug(BD_MANAGER, "serviceIRQ: End servicing IRQ\n");
}

BDVirtualDevice* BDManager::getDeviceByNumber(uint32 dev_num)
{
    auto found = eastl::find_if(device_list_.begin(), device_list_.end(), [dev_num](auto dev){ return dev->getDeviceNumber() == dev_num; });

    if (found != device_list_.end())
    {
        return *found;
    }

    return nullptr;
}

BDVirtualDevice* BDManager::getDeviceByName(const char * dev_name)
{
  if(!dev_name)
  {
      return 0;
  }

  debug(BD_MANAGER, "getDeviceByName: %s", dev_name);
  for (BDVirtualDevice* dev : device_list_)
  {
    if (strcmp(dev->getName(), dev_name) == 0)
    {
      debug(BD_MANAGER, "getDeviceByName: %s with id: %d\n", dev->getName(), dev->getDeviceNumber());
      return dev;
    }
  }
  return nullptr;
}

uint32 BDManager::getNumberOfDevices() const
{
  return device_list_.size();
}

BDManager* BDManager::instance_ = nullptr;
