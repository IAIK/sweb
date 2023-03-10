#pragma once

#include "types.h"
#include "paging-definitions.h"
#include "SpinLock.h"
#include "Bitmap.h"
#include "Allocator.h"
#include "BootstrapRangeAllocator.h"

// Allow KernelMemoryManager to allocate new pages from PageManager when kernel heap is full
// Please note that this means that the KMM depends on the page manager
// and you will have a harder time implementing swapping. Pros only!
static constexpr bool DYNAMIC_KMM = false;

/**
 * Responsible for allocation of physical memory pages
 */
class PageManager
{
public:
    PageManager() = delete;
    PageManager(const PageManager&) = delete;
    PageManager& operator=(const PageManager&) = delete;

    PageManager(Allocator* allocator);

    static PageManager& instance();
    static void init();
    static bool isReady();

    /**
     * Returns the total number of physical pages available to SWEB.
     * @return Number of available physical pages (4k page size)
     */
    uint32 getTotalNumPages() const;

    /**
     * Returns the number of currently free physical pages.
     * @return Number of free physical pages (4k page size)
     */
    size_t getNumFreePages() const;

    /**
     * Allocate a physical memory page
     * @param page_size The requested page size, must be a multiple of PAGE_SIZE (default = 4k)
     * @return The allocated physical page PPN
     */
    [[nodiscard("Discarding return value of allocPPN() leaks pages")]]
    uint32 allocPPN(uint32 page_size = PAGE_SIZE);

    /**
     * Releases the given physical page number (PPN) and marks it as free
     * @param page_number The physical page PPN to free
     * @param page_size The page size to free, must be a multiple of PAGE_SIZE
     */
    void freePPN(uint32 page_number, uint32 page_size = PAGE_SIZE);

    Thread* heldBy()
    {
      return lock_.heldBy();
    }

    void printUsageInfo() const
    {
      debug(PM, "Useable pages:\n");
      allocator_->printUsageInfo();
    }

private:
    static size_t initUsableMemoryRegions(Allocator& allocator);
    static void reserveKernelPages(Allocator& allocator);
    static void initFreePageCanaries(BootstrapRangeAllocator& allocator);

    void switchToHeapBitmapAllocator();

    Allocator* allocator_;

    size_t number_of_pages_;

    SpinLock lock_;

    static PageManager* instance_;

    static bool pm_ready_;
};
