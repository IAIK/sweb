#include "DeviceBus.h"

DeviceBus<>& deviceTreeRoot()
{
    static DeviceBus device_bus_root("Device Root");
    return device_bus_root;
}
