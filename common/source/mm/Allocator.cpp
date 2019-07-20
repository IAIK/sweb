#include "Allocator.h"
#include "debug.h"
#include "assert.h"
#include "paging-definitions.h"


/*
  Does not merge overlapping ranges when ranges are expanded!
 */
void BootstrapRangeAllocator::setUseable(__attribute__((unused)) size_t start, __attribute__((unused)) size_t end)
{
        //debug(PM, "setUseable [%zx - %zx)\n", start, end);
        assert(start <= end);
        for(size_t i = 0; i < sizeof(useable_ranges_)/sizeof(useable_ranges_[0]); ++i)
        {
                if((start >= useable_ranges_[i].start) &&
                   (end <= useable_ranges_[i].end))
                {
                        //debug(PM, "setUseable [%zx - %zx): already covered by [%zx - %zx)\n", start, end, useable_ranges_[i].start, useable_ranges_[i].end);
                        return;
                }
                else if((start <= useable_ranges_[i].start) &&
                        (end >= useable_ranges_[i].end))
                {
                        //debug(PM, "setUseable [%zx - %zx) completely covers [%zx - %zx)\n", start, end, useable_ranges_[i].start, useable_ranges_[i].end);
                        useable_ranges_[i].start = start;
                        useable_ranges_[i].end = end;
                        return;
                }
                else if((start >= useable_ranges_[i].start) &&
                        (start <= useable_ranges_[i].end))
                {
                        //debug(PM, "setUseable [%zx - %zx) expands end [%zx - %zx)\n", start, end, useable_ranges_[i].start, useable_ranges_[i].end);
                        useable_ranges_[i].end = Max(useable_ranges_[i].end, end);
                        return;
                }
                else if((end >= useable_ranges_[i].start) &&
                        (end <= useable_ranges_[i].end))
                {
                        //debug(PM, "setUseable [%zx - %zx) expands start [%zx - %zx)\n", start, end, useable_ranges_[i].start, useable_ranges_[i].end);
                        useable_ranges_[i].start = Min(useable_ranges_[i].start, start);
                        return;
                }
        }

        ssize_t slot = findFirstFreeSlot();
        assert(slot != -1);

        useable_ranges_[slot].start = start;
        useable_ranges_[slot].end = end;
}

void BootstrapRangeAllocator::setUnuseable(__attribute__((unused)) size_t start, __attribute__((unused)) size_t end)
{
        //debug(PM, "setUnuseable [%zx - %zx)\n", start, end);
        assert(start <= end);
        for(size_t i = 0; i < sizeof(useable_ranges_)/sizeof(useable_ranges_[0]); ++i)
        {
                if((start > useable_ranges_[i].start) &&
                        (end < useable_ranges_[i].end))
                {
                        //debug(PM, "setUnuseable [%zx - %zx) splits [%zx - %zx) into [%zx - %zx)+[%zx - %zx)\n", start, end, useable_ranges_[i].start, useable_ranges_[i].end, useable_ranges_[i].start, start, end, useable_ranges_[i].end);
                        size_t prev_end = useable_ranges_[i].end;
                        useable_ranges_[i].end = start;
                        setUseable(end, prev_end);
                }
                else if((end > useable_ranges_[i].start) &&
                        (end <= useable_ranges_[i].end))
                {
                        //debug(PM, "setUnuseable [%zx - %zx) moves start of [%zx - %zx)\n", start, end, useable_ranges_[i].start, useable_ranges_[i].end);
                        useable_ranges_[i].start = Min(end, useable_ranges_[i].end);
                }
                else if((start >= useable_ranges_[i].start) &&
                        (start < useable_ranges_[i].end))
                {
                        //debug(PM, "setUnuseable [%zx - %zx) moves end of [%zx - %zx)\n", start, end, useable_ranges_[i].start, useable_ranges_[i].end);
                        useable_ranges_[i].end = Min(start, useable_ranges_[i].end);
                }

        }
}

ssize_t BootstrapRangeAllocator::findFirstFreeSlot()
{
        for(size_t i = 0; i < sizeof(useable_ranges_)/sizeof(useable_ranges_[0]); ++i)
        {
                if(!slotIsUsed(i))
                {
                        return i;
                }
        }

        return -1;
}

bool BootstrapRangeAllocator::slotIsUsed(size_t i)
{
        assert(i < sizeof(useable_ranges_)/sizeof(useable_ranges_[0]));
        return useable_ranges_[i].start != useable_ranges_[i].end;
}

void BootstrapRangeAllocator::printUsageInfo()
{
        debug(PM, "Bootstrap PM useable ranges:\n");

        for(size_t i = 0; i < sizeof(useable_ranges_)/sizeof(useable_ranges_[0]); ++i)
        {
                if(slotIsUsed(i))
                {
                        debug(PM, "[%zx - %zx)\n", useable_ranges_[i].start, useable_ranges_[i].end);
                }
        }
}

size_t BootstrapRangeAllocator::numFree()
{
        size_t num_free = 0;
        for(size_t i = 0; i < sizeof(useable_ranges_)/sizeof(useable_ranges_[0]); ++i)
        {
                if(slotIsUsed(i))
                {
                        num_free += useable_ranges_[i].end - useable_ranges_[i].start;
                }
        }

        return num_free;
}

size_t BootstrapRangeAllocator::numUseablePages()
{
        size_t num_useable_pages = 0;
        for(size_t i = 0; i < sizeof(useable_ranges_)/sizeof(useable_ranges_[0]); ++i)
        {
                if(slotIsUsed(i))
                {
                        num_useable_pages += (useable_ranges_[i].end - useable_ranges_[i].start)/PAGE_SIZE;
                }
        }
        return num_useable_pages;
}

size_t BootstrapRangeAllocator::alloc(size_t size, size_t alignment)
{
        for(size_t i = 0; i < sizeof(useable_ranges_)/sizeof(useable_ranges_[0]); ++i)
        {
                size_t start = useable_ranges_[i].start;
                if(start % alignment != 0)
                {
                        start = start - (start % alignment) + alignment;
                }
                if(start + size <= useable_ranges_[i].end)
                {
                        //debug(PM, "Bootstrap PM allocating range [%zx-%zx)\n", start, start+size);
                        useable_ranges_[i].start = start + size;
                        return start;
                }
        }

        return -1;
}



bool BootstrapRangeAllocator::dealloc(size_t start, size_t size)
{
        // BootstrapRangeAllocator doesn't have error checking for deallocation
        setUseable(start, start + size);
        return true;
}
