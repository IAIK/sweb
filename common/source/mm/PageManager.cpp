#include "types.h"
#include "PageManager.h"
#include "new.h"
#include "offsets.h"
#include "paging-definitions.h"
#include "ArchCommon.h"
#include "ArchMemory.h"
#include "kprintf.h"
#include "kstring.h"
#include "Scheduler.h"
#include "KernelMemoryManager.h"
#include "assert.h"
#include "Bitmap.h"
#include "Allocator.h"
#include "BitmapAllocator.h"
#include "BootloaderModules.h"

extern void* kernel_start_address;
extern void* kernel_end_address;

PageManager* PageManager::instance_ = nullptr;
bool PageManager::pm_ready_ = false;

PageManager& PageManager::instance()
{
  assert(instance_ && "PageManager not yet initialized");
  return *instance_;
}

void PageManager::init()
{
    assert(!instance_ && "PageManager already initialized");
    BootstrapRangeAllocator bootstrap_pm_allocator{};

    static PageManager instance(&bootstrap_pm_allocator);
    instance_ = &instance;

    ArchCommon::reservePagesPreKernelInit(bootstrap_pm_allocator);

    initFreePageCanaries(bootstrap_pm_allocator);

    // Need to do this while bootstrap allocator is still valid/alive
    KernelMemoryManager::init();
    instance_->switchToHeapBitmapAllocator();
}

void PageManager::initFreePageCanaries(BootstrapRangeAllocator& allocator)
{
    debug(PM, "Filling free pages with canary value\n");
    for (auto paddr : allocator.freeBlocks(PAGE_SIZE, PAGE_SIZE))
    {
        ppn_t ppn = paddr/PAGE_SIZE;
        memset((void*)ArchMemory::getIdentAddressOfPPN(ppn), 0xFF, PAGE_SIZE);
    }
}

bool PageManager::isReady()
{
    return pm_ready_;
}


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

  size_t total_num_useable_pages = allocator_->numFreeBlocks(PAGE_SIZE, PAGE_SIZE);
  number_of_pages_ = highest_address / PAGE_SIZE;

  debug(PM, "Bootstrap PM useable ranges:\n");
  allocator_->printUsageInfo();
  debug(PM, "Total useable pages: %zu (%zu bytes)\n", total_num_useable_pages, total_num_useable_pages*PAGE_SIZE);

  reserveKernelPages(*allocator_);
  BootloaderModules::reserveModulePages(*allocator_);

  debug(PM, "Ctor: Physical pages - free: %zu used: %zu total: %zu\n", getNumFreePages(), total_num_useable_pages - getNumFreePages(), total_num_useable_pages);

  pm_ready_ = true;
  debug(PM, "PM ctor finished\n");
}


uint32 PageManager::getTotalNumPages() const
{
  return number_of_pages_;
}


size_t PageManager::getNumFreePages() const
{
  return allocator_->numFreeBlocks(PAGE_SIZE, PAGE_SIZE);
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

  debugAdvanced(PM, "Allocated PPN %llx\n", (long long unsigned)ppn);

  char* page_ident_addr = (char*)ArchMemory::getIdentAddressOfPPN(ppn);
  const char* page_modified = (const char*)memnotchr(page_ident_addr, 0xFF, page_size);
  if(page_modified)
  {
      debugAlways(PM, "Detected use-after-free for PPN %llx at offset %zx\n", (long long unsigned)ppn, page_modified - page_ident_addr);
      assert(!page_modified && "Page modified after free");
  }

  memset(page_ident_addr, 0, page_size);
  return ppn;
}


void PageManager::freePPN(uint32 page_number, uint32 page_size)
{
  debugAdvanced(PM, "Free PPN %x\n", page_number);

  assert((page_size % PAGE_SIZE) == 0);
  assert(allocator_);
  if(page_number > getTotalNumPages())
  {
    debugAlways(PM, "Tried to free nonexistent PPN %x\n", page_number);
    assert(false && "PPN to be freed is out of range");
  }

  // Fill page with 0xFF for use-after-free detection
  memset((void*)ArchMemory::getIdentAddressOfPPN(page_number), 0xFF, page_size);

  lock_.acquire();
  bool free_status = allocator_->dealloc(page_number*PAGE_SIZE, page_size);
  if (!free_status)
  {
      debugAlways(PM, "Double free PPN %x\n", page_number);
      assert(false && "Double free PPN");
  }
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

void PageManager::switchToHeapBitmapAllocator()
{
    debug(PM, "Allocating PM bitmap with %#zx bits\n", number_of_pages_);
    allocator_ = new BitmapAllocator<PAGE_SIZE>(number_of_pages_*PAGE_SIZE, eastl::move(*allocator_));
}
