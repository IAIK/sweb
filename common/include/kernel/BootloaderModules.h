#pragma once

#include "Allocator.h"

class BDVirtualDevice;

namespace BootloaderModules
{
    void reserveModulePages(Allocator& allocator);
    void mapModules();

    BDVirtualDevice* createRamDiskFromModule(int module_num, const char* name);
    void loadInitrdIfExists();
};
