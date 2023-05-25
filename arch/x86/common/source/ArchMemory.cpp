#include "ArchMemory.h"

#include "SMP.h"
#include "offsets.h"

#include "ArchCommon.h"
#include "ArchInterrupts.h"
#include "ArchMulticore.h"

#include "debug.h"

// Common functionality for x86 memory management (x86_64 / x86_32 / x86_32_pae)

pointer ArchMemory::getIdentAddressOfPPN(ppn_t ppn, size_t page_size /* optional */)
{
    return IDENT_MAPPING_START + (ppn * page_size);
}

pointer ArchMemory::getIdentAddress(size_t address)
{
    return IDENT_MAPPING_START | (address);
}

size_t ArchMemory::getKernelPagingStructureRootPhys()
{
    return (size_t)VIRTUAL_TO_PHYSICAL_BOOT(getKernelPagingStructureRootVirt());
}

size_t ArchMemory::getValueForCR3() const
{
    return getPagingStructureRootPhys();
}

void ArchMemory::loadPagingStructureRoot(size_t cr3_value)
{
    __asm__ __volatile__("mov %[cr3_value], %%cr3\n"
                         ::[cr3_value]"r"(cr3_value));
}

void ArchMemory::flushLocalTranslationCaches(size_t addr)
{
    if(A_MEMORY & OUTPUT_ADVANCED)
    {
        debug(A_MEMORY, "CPU %zx flushing translation caches for address %zx\n", SMP::currentCpuId(), addr);
    }
    __asm__ __volatile__("invlpg %[addr]\n"
                         ::[addr]"m"(*(char*)addr));
}

void ArchMemory::flushAllTranslationCaches(size_t addr)
{
    flushLocalTranslationCaches(addr);

    SMP::callOnOtherCpus([addr]{ flushLocalTranslationCaches(addr); });
}
