#include "types.h"
#include "PageManager.h"
#include "new.h"
#include "offsets.h"
#include "paging-definitions.h"
#include "ArchCommon.h"
#include "ArchMemory.h"
#include "debug_bochs.h"
#include "kprintf.h"
#include "Scheduler.h"
#include "ArchInterrupts.h"
#include "KernelMemoryManager.h"
#include "assert.h"
#include "Bitmap.h"
#include "Allocator.h"

alignas(PageManager) unsigned char pm[sizeof(PageManager)];

PageManager* PageManager::instance_ = nullptr;

PageManager* PageManager::instance()
{
  assert(instance_);
  return instance_;
}

void PageManager::init()
{
    assert(!instance_);
    BootstrapRangeAllocator bootstrap_pm_allocator{};
    instance_ = new (&pm) PageManager(&bootstrap_pm_allocator);

    // HACKY: Pages 0 + 1 are used for AP startup code
    size_t ap_boot_code_range = instance_->allocator_->alloc(PAGE_SIZE*2, PAGE_SIZE);
    assert(ap_boot_code_range == 0);

    initKernelMemoryManager();
    instance_->switchToHeapBitmapAllocator();
}

extern void* kernel_start_address;
extern void* kernel_end_address;
extern uint8 boot_stack[0x4000];

enum MemType
{
        M_USEABLE = 1,
        M_ACPI_INFO = 3,
        M_RESERVED_PRESERVE_ON_HIBERNATION = 4,
        M_BAD = 5,
};

const char* multiboot_memtype[] = {
        "reserved",
        "available RAM",
        "reserved",
        "ACPI information",
        "reserved (preserve on hibernation)",
        "bad memory",
};

const char* getMultibootMemTypeString(size_t type)
{
        return (type < sizeof(multiboot_memtype)/sizeof(char*) ? multiboot_memtype[type] : "reserved");
}


PageManager::PageManager(Allocator* allocator) :
        allocator_(allocator),
        number_of_pages_(0),
        lock_("PageManager::lock_")
{
  size_t highest_address = initUsableMemoryRegions(*allocator_);

  size_t total_num_useable_pages = allocator_->numFreeContiguousBlocks(PAGE_SIZE, PAGE_SIZE);
  number_of_pages_ = highest_address / PAGE_SIZE;

  allocator_->printUsageInfo();
  debug(PM, "Total useable pages: %zu (%zu bytes)\n", total_num_useable_pages, total_num_useable_pages*PAGE_SIZE);

  reserveKernelPages(*allocator_);
  reserveModulePages(*allocator_);

  debug(PM, "Ctor: Physical pages - free: %zu used: %zu total: %zu\n", getNumFreePages(), total_num_useable_pages - getNumFreePages(), total_num_useable_pages);

  KernelMemoryManager::pm_ready_ = 1;
  debug(PM, "PM ctor finished\n");
}


uint32 PageManager::getTotalNumPages() const
{
  return number_of_pages_;
}


size_t PageManager::getNumFreePages() const
{
  return allocator_->numFreeContiguousBlocks(PAGE_SIZE, PAGE_SIZE);
  // return allocator_->numFree()/PAGE_SIZE;
}


uint32 PageManager::allocPPN(uint32 page_size)
{
  assert((page_size % PAGE_SIZE) == 0);
  assert(allocator_);

  lock_.acquire();
  size_t phys_addr = allocator_->alloc(page_size, page_size);
  lock_.release();

  if (phys_addr == (size_t)-1)
  {
    assert(false && "PageManager::allocPPN: Out of memory / No more free physical pages");
  }

  ppn_t ppn = phys_addr/PAGE_SIZE;
  memset((void*)ArchMemory::getIdentAddressOfPPN(ppn), 0, page_size);
  return ppn;
}


void PageManager::freePPN(uint32 page_number, uint32 page_size)
{
  assert((page_size % PAGE_SIZE) == 0);
  assert(allocator_);

  lock_.acquire();
  bool free_status = allocator_->dealloc(page_number*PAGE_SIZE, page_size);
  assert(free_status && "Double free PPN");
  lock_.release();
}

size_t PageManager::initUsableMemoryRegions(Allocator& allocator)
{
    size_t num_mmaps = ArchCommon::getNumUseableMemoryRegions();

    size_t highest_address = 0;

    //Determine Amount of RAM
    for (size_t i = 0; i < num_mmaps; ++i)
    {
        pointer start_address = 0, end_address = 0;
        size_t type = 0;
        ArchCommon::getUseableMemoryRegion(i, start_address, end_address, type);
        assert(type <= 5);
        debug(PM, "Ctor: memory region from physical %#zx to %#zx (%zu bytes) of type %zd [%s]\n",
              start_address, end_address, end_address - start_address, type, getMultibootMemTypeString(type));

        if (type == M_USEABLE)
        {
            highest_address = Max(highest_address, end_address);
            allocator.setUseable(start_address, end_address);
        }
    }

    return highest_address;
}

void PageManager::reserveKernelPages(Allocator& allocator)
{
    debug(PM, "Marking pages used by the kernel as reserved\n");
    size_t kernel_virt_start = (size_t)&kernel_start_address;
    size_t kernel_virt_end   = (size_t)&kernel_end_address;
    size_t kernel_phys_start = kernel_virt_start - (size_t)PHYSICAL_TO_VIRTUAL_OFFSET;
    size_t kernel_phys_end   = kernel_virt_end - (size_t)PHYSICAL_TO_VIRTUAL_OFFSET;
    debug(PM, "Ctor: kernel phys [%#zx, %#zx) -> virt [%#zx, %#zx)\n", kernel_phys_start, kernel_phys_end, kernel_virt_start, kernel_virt_end);

    allocator.setUnuseable(kernel_phys_start, kernel_phys_end);
}

void PageManager::reserveModulePages(Allocator& allocator)
{
    debug(PM, "Ctor: Marking GRUB loaded modules as reserved\n");
    for (size_t i = 0; i < ArchCommon::getNumModules(); ++i)
    {
        size_t module_phys_start = (ArchCommon::getModuleStartAddress(i) - (size_t)PHYSICAL_TO_VIRTUAL_OFFSET);
        size_t module_phys_end = (ArchCommon::getModuleEndAddress(i) - (size_t)PHYSICAL_TO_VIRTUAL_OFFSET);
        debug(PM, "Ctor: module [%s]: phys [%p, %p)\n", ArchCommon::getModuleName(i), (void*)module_phys_start, (void*)module_phys_end);
        if(module_phys_end < module_phys_start)
            continue;

        allocator.setUnuseable(module_phys_start, module_phys_end);
    }
}

void PageManager::mapModules()
{
  debug(PM, "Ctor: Mapping GRUB loaded modules\n");
  for (size_t i = 0; i < ArchCommon::getNumModules(); ++i)
  {
    size_t module_phys_start = (ArchCommon::getModuleStartAddress(i) - (size_t)PHYSICAL_TO_VIRTUAL_OFFSET);
    size_t module_phys_end = (ArchCommon::getModuleEndAddress(i) - (size_t)PHYSICAL_TO_VIRTUAL_OFFSET);
    debug(PM, "Ctor: module [%s]: virt [%p, %p), phys [%p, %p)\n", ArchCommon::getModuleName(i), (void*)ArchCommon::getModuleStartAddress(i), (void*)ArchCommon::getModuleEndAddress(i), (void*)module_phys_start, (void*)module_phys_end);
    if(module_phys_end < module_phys_start)
      continue;

    size_t start_page = module_phys_start / PAGE_SIZE;
    size_t end_page = (module_phys_end + PAGE_SIZE-1) / PAGE_SIZE;
    for (size_t k = start_page; k < Min(end_page, number_of_pages_); ++k)
    {
      if(ArchMemory::checkAddressValid(((size_t)VIRTUAL_TO_PHYSICAL_BOOT(ArchMemory::getRootOfKernelPagingStructure()) / PAGE_SIZE), PHYSICAL_TO_VIRTUAL_OFFSET + k*PAGE_SIZE))
      {
        debug(PM, "Cannot map kernel module at %#zx, already mapped\n", k*PAGE_SIZE);
        continue;
      }

        if(PM & OUTPUT_ADVANCED)
          debug(PM, "Mapping kernel module at %#zx -> %#zx\n", (size_t)PHYSICAL_TO_VIRTUAL_OFFSET / PAGE_SIZE + k, k);

        ArchMemory::mapKernelPage(PHYSICAL_TO_VIRTUAL_OFFSET / PAGE_SIZE + k, k, true);
    }
  }
  debug(PM, "Finished mapping modules\n");
}

size_t PageManager::calcNumHeapPages(Allocator& allocator)
{
    size_t HEAP_PAGES = allocator.numFreeContiguousBlocks(PAGE_SIZE, PAGE_SIZE)/3;
    if (HEAP_PAGES > 1024)
        HEAP_PAGES = 1024 + (HEAP_PAGES - Min(HEAP_PAGES, 1024))/8;
    return HEAP_PAGES;
}

size_t PageManager::mapKernelHeap(Allocator& allocator, size_t max_heap_pages)
{
  debug(PM, "Before kernel heap allocation:\n");
  allocator.printUsageInfo();
  debug(PM, "Num free pages: %zu\n", allocator.numFreeContiguousBlocks(PAGE_SIZE, PAGE_SIZE));

  debug(PM, "Mapping %zu reserved kernel heap pages\n", max_heap_pages);
  size_t num_reserved_heap_pages = 0;
  for (size_t kheap_vpn = ArchCommon::getFreeKernelMemoryStart() / PAGE_SIZE; num_reserved_heap_pages < max_heap_pages; ++num_reserved_heap_pages, ++kheap_vpn)
  {
    if(ArchMemory::checkAddressValid(((size_t)VIRTUAL_TO_PHYSICAL_BOOT(ArchMemory::getRootOfKernelPagingStructure()) / PAGE_SIZE), kheap_vpn*PAGE_SIZE))
    {
      debug(PM, "Cannot map vpn %#zx for kernel heap, already mapped\n", kheap_vpn);
      break;
    }

    ppn_t ppn_to_map = allocPPN();
    if(PM & OUTPUT_ADVANCED)
      debug(PM, "Mapping kernel heap vpn %p -> ppn %p\n", (void*)kheap_vpn, (void*)ppn_to_map);
    ArchMemory::mapKernelPage(kheap_vpn, ppn_to_map, true);
  }
  debug(PM, "Finished mapping kernel heap [%zx - %zx), initializing KernelMemoryManager\n",
        ArchCommon::getFreeKernelMemoryStart(), ArchCommon::getFreeKernelMemoryStart() + num_reserved_heap_pages*PAGE_SIZE);
  return num_reserved_heap_pages;
}

void PageManager::initKernelMemoryManager()
{
    assert(KernelMemoryManager::instance_ == nullptr);
    size_t max_heap_pages = calcNumHeapPages(*instance_->allocator_);
    size_t num_reserved_heap_pages = instance_->mapKernelHeap(*instance_->allocator_, max_heap_pages);
    extern KernelMemoryManager kmm;
    KernelMemoryManager::instance_ = new (&kmm) KernelMemoryManager(num_reserved_heap_pages, max_heap_pages);
}

void PageManager::switchToHeapBitmapAllocator()
{
    debug(PM, "Allocating PM bitmap with %#zx bits\n", number_of_pages_);
    allocator_ = new BitmapAllocator<PAGE_SIZE>(number_of_pages_*PAGE_SIZE, ustl::move(*allocator_));
}
