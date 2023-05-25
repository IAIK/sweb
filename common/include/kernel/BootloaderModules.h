#pragma once

class BDVirtualDevice;
class Allocator;

namespace BootloaderModules
{
    void reserveModulePages(Allocator& allocator);
    void mapModules();

    BDVirtualDevice* createRamDiskFromModule(int module_num, const char* name);
    void loadInitrdIfExists();
};
