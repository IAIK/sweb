#include "DeviceBus.h"

DeviceBus& DeviceBus::root()
{
    static DeviceBus device_bus_root("Device Root");
    return device_bus_root;
}
