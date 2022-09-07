#include "BootstrapRangeAllocator.h"
#include "debug.h"
#include "assert.h"
#include "paging-definitions.h"

BootstrapRangeAllocator::BootstrapRangeAllocator()
{
    iset_.get_allocator().init(iset_buffer_,
                               sizeof(iset_buffer_),
                               sizeof(decltype(iset_)::node_type),
                               alignof(decltype(iset_)::node_type));
}

void BootstrapRangeAllocator::setUseable(size_t start, size_t end)
{
        assert(start <= end);

        iset_.insert({start, end});
}

void BootstrapRangeAllocator::setUnuseable(size_t start, size_t end)
{
        assert(start <= end);

        iset_.erase({start, end});
}

void BootstrapRangeAllocator::printUsageInfo() const
{
        debug(PM, "Bootstrap PM useable ranges:\n");

        for (const auto& interval : iset_)
        {
            debug(PM, "[%zx - %zx)\n", interval.first, interval.second);
        }
}

size_t BootstrapRangeAllocator::numFree() const
{
        size_t num_free = 0;
        for (const auto& iv : iset_)
        {
            num_free += iv.second - iv.first;
        }

        return num_free;
}

size_t BootstrapRangeAllocator::numFreeBlocks(size_t size, size_t alignment) const
{
    assert(size > 0);
    assert(alignment == size);

    size_t num_blocks = 0;
    for (auto& iv : iset_)
    {
        size_t aligned_start = iv.first;
        aligned_start += (aligned_start % alignment ? alignment - aligned_start % alignment : 0);
        num_blocks += (iv.second - aligned_start)/size;
    }

    return num_blocks;
}


size_t BootstrapRangeAllocator::alloc(size_t size, size_t alignment)
{
    for (const auto& iv : iset_)
    {
        size_t start = iv.first;
        size_t align_offset = start % alignment;
        start += (align_offset ? alignment - align_offset : 0);

        if(start + size <= iv.second)
        {
            iset_.erase({start, start + size});
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


size_t BootstrapRangeAllocator::nextFreeBlock(size_t size, size_t alignment, size_t start) const
{
    for (auto& iv : iset_)
    {
        size_t check_start = start < iv.first ? iv.first : start;
        size_t align_offset = check_start % alignment;
        check_start += (align_offset ? alignment - align_offset : 0);
        if(check_start + size <= iv.second)
        {
            return check_start;
        }
    }

    return -1;
}
