#pragma once

#include "Allocator.h"
#include "IntervalSet.h"
#include "ranges.h"

#include <EASTL/fixed_allocator.h>

template<typename TAllocator = EASTLAllocatorType>
class RangeAllocator : public Allocator
{
public:
    RangeAllocator() = default;

    template<typename U>
    RangeAllocator(U&& iset_allocator) :
        iset_(eastl::forward<U>(iset_allocator))
    {
    }

    RangeAllocator(const RangeAllocator&) = delete;
    ~RangeAllocator() override = default;

    size_t alloc(size_t size, size_t alignment = 1) override
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

    bool dealloc(size_t start, size_t size) override
    {
        // RangeAllocator doesn't have error checking for deallocation
        setUseable(start, start + size);
        return true;
    }

    void setUseable(size_t start, size_t end) override
    {
        debug(PM, "Set useable [%zx - %zx)\n", start, end);
        assert(start <= end);
        iset_.insert({start, end});
    }

    void setUnuseable(size_t start, size_t end) override
    {
        debug(PM, "Set unuseable [%zx - %zx)\n", start, end);
        assert(start <= end);
        iset_.erase({start, end});
    }

    [[nodiscard]] size_t numFree() const override
    {
        size_t num_free = 0;
        for (const auto& iv : iset_)
        {
            num_free += iv.second - iv.first;
        }

        return num_free;
    }

    [[nodiscard]] size_t numFreeBlocks(size_t size, size_t alignment = 1) const override
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

    void printUsageInfo() const override
    {
        for (const auto& interval : iset_)
        {
            debug(PM, "[%zx - %zx)\n", interval.first, interval.second);
        }
    }

    [[nodiscard]] size_t nextFreeBlock(size_t size, size_t alignment, size_t start) const
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


    class AllocBlockIterator : public eastl::iterator<eastl::input_iterator_tag, size_t>
    {
    public:
        using iterator_category = eastl::input_iterator_tag;
        using value_type        = size_t;
        using pointer           = value_type*;
        using const_pointer     = const value_type*;
        using reference         = value_type&;
        using const_reference   = const value_type&;

        AllocBlockIterator(const RangeAllocator* allocator, size_t size, size_t alignment, size_t start) :
            allocator_(allocator), size_(size), alignment_(alignment), curr_(start) {}

        const_reference operator*() const { return curr_;  }
        const_pointer operator->()  const { return &curr_; }

        AllocBlockIterator& operator++() { curr_ = allocator_->nextFreeBlock(size_, alignment_, curr_ + size_); return *this; }

        friend bool operator== (const AllocBlockIterator& a, const AllocBlockIterator& b) { return a.curr_ == b.curr_; }
        friend bool operator!= (const AllocBlockIterator& a, const AllocBlockIterator& b) { return a.curr_ != b.curr_; }

    private:
        const RangeAllocator* allocator_;

        size_t size_;
        size_t alignment_;
        size_t curr_;
    };

    [[nodiscard]] auto freeBlocks(size_t size, size_t alignment) const
    {
        return ranges::subrange{freeBlocksBegin(size, alignment),
                                freeBlocksEnd(size, alignment)};
    }

protected:
    [[nodiscard]] AllocBlockIterator freeBlocksBegin(size_t size, size_t alignment) const
    {
        return AllocBlockIterator(this, size, alignment,
                                  nextFreeBlock(size, alignment, 0));
    }

    [[nodiscard]] AllocBlockIterator freeBlocksEnd(size_t size, size_t alignment) const
    {
        return AllocBlockIterator(this, size, alignment, -1);
    }

    IntervalSet<size_t, TAllocator> iset_;
};
