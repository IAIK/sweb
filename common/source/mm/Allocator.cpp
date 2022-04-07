#include "Allocator.h"
#include "debug.h"
#include "assert.h"
#include "paging-definitions.h"


/*
  Does not merge overlapping ranges when ranges are expanded!
 */
void BootstrapRangeAllocator::setUseable(size_t start, size_t end)
{
        //debug(PM, "setUseable [%zx - %zx)\n", start, end);
        assert(start <= end);
        for(auto & range : useable_ranges_)
        {
                if((start >= range.start) &&
                   (end <= range.end))
                {
                        //debug(PM, "setUseable [%zx - %zx): already covered by [%zx - %zx)\n", start, end, useable_ranges_[i].start, useable_ranges_[i].end);
                        return;
                }
                else if((start <= range.start) &&
                        (end >= range.end))
                {
                        //debug(PM, "setUseable [%zx - %zx) completely covers [%zx - %zx)\n", start, end, useable_ranges_[i].start, useable_ranges_[i].end);
                        range.start = start;
                        range.end = end;
                        return;
                }
                else if((start >= range.start) &&
                        (start <= range.end))
                {
                        //debug(PM, "setUseable [%zx - %zx) expands end [%zx - %zx)\n", start, end, useable_ranges_[i].start, useable_ranges_[i].end);
                        range.end = Max(range.end, end);
                        return;
                }
                else if((end >= range.start) &&
                        (end <= range.end))
                {
                        //debug(PM, "setUseable [%zx - %zx) expands start [%zx - %zx)\n", start, end, useable_ranges_[i].start, useable_ranges_[i].end);
                        range.start = Min(range.start, start);
                        return;
                }
        }

        ssize_t slot = findFirstFreeSlot();
        assert(slot != -1);

        useable_ranges_[slot].start = start;
        useable_ranges_[slot].end = end;
}

void BootstrapRangeAllocator::setUnuseable(size_t start, size_t end)
{
        //debug(PM, "setUnuseable [%zx - %zx)\n", start, end);
        assert(start <= end);
        for(auto & range : useable_ranges_)
        {
                if((start > range.start) &&
                        (end < range.end))
                {
                        //debug(PM, "setUnuseable [%zx - %zx) splits [%zx - %zx) into [%zx - %zx)+[%zx - %zx)\n", start, end, useable_ranges_[i].start, useable_ranges_[i].end, useable_ranges_[i].start, start, end, useable_ranges_[i].end);
                        size_t prev_end = range.end;
                        range.end = start;
                        setUseable(end, prev_end);
                }
                else if((end > range.start) &&
                        (end <= range.end))
                {
                        //debug(PM, "setUnuseable [%zx - %zx) moves start of [%zx - %zx)\n", start, end, useable_ranges_[i].start, useable_ranges_[i].end);
                        range.start = Min(end, range.end);
                }
                else if((start >= range.start) &&
                        (start < range.end))
                {
                        //debug(PM, "setUnuseable [%zx - %zx) moves end of [%zx - %zx)\n", start, end, useable_ranges_[i].start, useable_ranges_[i].end);
                        range.end = Min(start, range.end);
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

size_t BootstrapRangeAllocator::numFreeContiguousBlocks(size_t size, size_t alignment)
{
    assert(size > 0);
    assert(alignment == size);
    size_t num_contiguous_blocks = 0;
    for(size_t i = 0; i < sizeof(useable_ranges_)/sizeof(useable_ranges_[0]); ++i)
    {
        if(slotIsUsed(i))
        {
            size_t aligned_start = useable_ranges_[i].start;
            aligned_start += (aligned_start % alignment ? alignment - aligned_start % alignment : 0);
            num_contiguous_blocks += (useable_ranges_[i].end - aligned_start)/size;
        }
    }
    return num_contiguous_blocks;
}

size_t BootstrapRangeAllocator::alloc(size_t size, size_t alignment)
{
        for(auto & range : useable_ranges_)
        {
                size_t start = range.start;
                size_t align_offset = start % alignment;
                start += (align_offset ? alignment - align_offset : 0);

                if(start + size <= range.end)
                {
                        //debug(PM, "Bootstrap PM allocating range [%zx-%zx)\n", start, start+size);
                        range.start = start + size;
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

size_t BootstrapRangeAllocator::nextFreeBlock(size_t size, size_t alignment, size_t start)
{
    for(auto & range : useable_ranges_)
    {
        size_t check_start = start < range.start ? range.start : start;
        size_t align_offset = check_start % alignment;
        check_start += (align_offset ? alignment - align_offset : 0);
        if(check_start + size <= range.end)
        {
            return check_start;
        }
    }

    return -1;
}
