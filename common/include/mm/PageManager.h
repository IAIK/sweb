#pragma once

#include "types.h"
#include "paging-definitions.h"
#include "SpinLock.h"
#include "Bitmap.h"
#include "Allocator.h"
#include "BootstrapRangeAllocator.h"

#define DYNAMIC_KMM (0) // Please note that this means that the KMM depends on the page manager
// and you will have a harder time implementing swapping. Pros only!

class PageManager
{
  public:
    PageManager() = delete;
    PageManager(PageManager const&) = delete;
    PageManager& operator=(const PageManager&) = delete;

    PageManager(Allocator* allocator);

    static PageManager *instance();
    static void init();
    static bool isReady();

    /**
     * returns the number of 4k Pages avaible to sweb.
     * ((size of the first usable memory region or max 1Gb) / PAGESIZE)
     * @return number of available pages
     */
    uint32 getTotalNumPages() const;

    /**
     * returns the number of currently free pages
     * @return number of free pages
     */
    size_t getNumFreePages() const;

    /**
     * returns the number of the lowest free Page
     * and marks that Page as used.
     * returns always 4kb ppns!
     */
    [[nodiscard("Discarding return value of allocPPN() leaks pages")]]
    uint32 allocPPN(uint32 page_size = PAGE_SIZE);

    /**
     * marks physical page <page_number> as free, if it was used in
     * user or kernel space.
     * @param page_number Physcial Page to mark as unused
     */
    void freePPN(uint32 page_number, uint32 page_size = PAGE_SIZE);

    Thread* heldBy()
    {
      return lock_.heldBy();
    }

    void printUsageInfo() const
    {
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
