#include "BootloaderModules.h"
#include "debug.h"
#include "PageManager.h"
#include "ArchCommon.h"
#include "ArchMemory.h"
#include "offsets.h"

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
            if(ArchMemory::checkAddressValid(((size_t)VIRTUAL_TO_PHYSICAL_BOOT(ArchMemory::getRootOfKernelPagingStructure()) / PAGE_SIZE), PHYSICAL_TO_VIRTUAL_OFFSET + k*PAGE_SIZE))
            {
                debug(MAIN, "Cannot map kernel module at %#zx, already mapped\n", k*PAGE_SIZE);
                continue;
            }

            if(MAIN & OUTPUT_ADVANCED)
                debug(MAIN, "Mapping kernel module at %#zx -> %#zx\n", (size_t)PHYSICAL_TO_VIRTUAL_OFFSET / PAGE_SIZE + k, k);

            ArchMemory::mapKernelPage(PHYSICAL_TO_VIRTUAL_OFFSET / PAGE_SIZE + k, k, true);
        }
    }
    debug(MAIN, "Finished mapping modules\n");
}
