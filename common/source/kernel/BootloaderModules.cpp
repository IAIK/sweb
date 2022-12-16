#include "BootloaderModules.h"
#include "debug.h"
#include "PageManager.h"
#include "ArchCommon.h"
#include "ArchMemory.h"
#include "offsets.h"
#include "RamDiskDriver.h"
#include "BDManager.h"
#include "BDVirtualDevice.h"

void BootloaderModules::reserveModulePages(Allocator& allocator)
{
    debug(MAIN, "Marking bootloader loaded modules as reserved\n");
    for (size_t i = 0; i < ArchCommon::getNumModules(); ++i)
    {
        size_t module_phys_start = (ArchCommon::getModuleStartAddress(i) - (size_t)PHYSICAL_TO_VIRTUAL_OFFSET);
        size_t module_phys_end = (ArchCommon::getModuleEndAddress(i) - (size_t)PHYSICAL_TO_VIRTUAL_OFFSET);
        debug(MAIN, "module [%s]: phys [%p, %p)\n", ArchCommon::getModuleName(i), (void*)module_phys_start, (void*)module_phys_end);
        if(module_phys_end < module_phys_start)
            continue;

        allocator.setUnuseable(module_phys_start, module_phys_end);
    }
}

void BootloaderModules::mapModules()
{
    debug(MAIN, "Mapping bootloader loaded modules\n");
    for (size_t i = 0; i < ArchCommon::getNumModules(); ++i)
    {
        size_t module_phys_start = (ArchCommon::getModuleStartAddress(i) - (size_t)PHYSICAL_TO_VIRTUAL_OFFSET);
        size_t module_phys_end = (ArchCommon::getModuleEndAddress(i) - (size_t)PHYSICAL_TO_VIRTUAL_OFFSET);
        debug(MAIN, "module [%s]: virt [%p, %p), phys [%p, %p)\n", ArchCommon::getModuleName(i), (void*)ArchCommon::getModuleStartAddress(i), (void*)ArchCommon::getModuleEndAddress(i), (void*)module_phys_start, (void*)module_phys_end);
        if(module_phys_end < module_phys_start)
            continue;

        size_t start_page = module_phys_start / PAGE_SIZE;
        size_t end_page = (module_phys_end + PAGE_SIZE-1) / PAGE_SIZE;
        for (size_t k = start_page; k < Min(end_page, PageManager::instance()->getTotalNumPages()); ++k)
        {
            if(MAIN & OUTPUT_ADVANCED)
                debug(MAIN, "Mapping kernel module at %#zx -> %#zx\n", (size_t)PHYSICAL_TO_VIRTUAL_OFFSET / PAGE_SIZE + k, k);

            bool mapped = ArchMemory::mapKernelPage(PHYSICAL_TO_VIRTUAL_OFFSET / PAGE_SIZE + k, k, true);
            if (!mapped)
            {
                debug(MAIN, "Cannot map kernel module at %#zx, already mapped\n", k*PAGE_SIZE);
            }
        }
    }
    debug(MAIN, "Finished mapping modules\n");
}


BDVirtualDevice* BootloaderModules::createRamDiskFromModule(int module_num, const char* name)
{
    size_t ramdisk_size = ArchCommon::getModuleEndAddress(module_num) - ArchCommon::getModuleStartAddress(module_num);
    debug(MAIN, "Creating ram disk from module %s at [%zx, %zx), size: %zx\n", ArchCommon::getModuleName(module_num), ArchCommon::getModuleStartAddress(module_num), ArchCommon::getModuleEndAddress(module_num), ramdisk_size);
    return RamDiskDriver::createRamDisk((void*)ArchCommon::getModuleStartAddress(module_num), ramdisk_size, name);
}

void BootloaderModules::loadInitrdIfExists()
{
    // TODO: ArchCommon::getModuleEndAddress(i) -> getKernelEndAddress() crashes on arm rpi2

    for(size_t i = 0; i < ArchCommon::getNumModules(); ++i)
    {
        debug(MAIN, "Checking module %zu: %s\n", i, ArchCommon::getModuleName(i));

        if(strcmp(ArchCommon::getModuleName(i), "/boot/initrd") == 0)
        {
            debug(MAIN, "Initialize initrd\n");
            BDVirtualDevice* initrd_dev = createRamDiskFromModule(i, "initrd");
            initrd_dev->setPartitionType(0x81);
            BDManager::instance().addVirtualDevice(initrd_dev);
            break;
        }
    }
}
