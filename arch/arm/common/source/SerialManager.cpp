#include "ArchSerialInfo.h"
#include "SerialManager.h"
#include "kstring.h"

#include "debug_bochs.h"
#include "kprintf.h"


SerialManager::SerialManager() :
    BasicDeviceDriver("Serial Port Driver"),
    num_ports(0)
{
}

void SerialManager::doDeviceDetection()
{
}

uint32 SerialManager::get_num_ports() const
{
  return num_ports;
}

uint32 SerialManager::do_detection([[maybe_unused]]uint32 is_paging_set_up)
{
  return num_ports;
}

void SerialManager::service_irq([[maybe_unused]]uint32 irq_num)
{
}
