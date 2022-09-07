#pragma once
#include "Allocator.h"
#include "IntervalSet.h"
#include <EASTL/fixed_allocator.h>


/* This BootstrapRangeAllocator is 'good enough' for temporary use during initialization
 * of the page manager, but should not really be used elsewhere
 */
class BootstrapRangeAllocator : public Allocator
{
public:
    BootstrapRangeAllocator();
    BootstrapRangeAllocator(const BootstrapRangeAllocator&) = delete;
    virtual ~BootstrapRangeAllocator() = default;

    virtual size_t alloc(size_t size, size_t alignment = 1);
    virtual bool dealloc(size_t start, size_t size);

    virtual void setUseable(size_t start, size_t end);
    virtual void setUnuseable(size_t start, size_t end);

    virtual size_t numFree() const;
    virtual size_t numFreeBlocks(size_t size, size_t alignment = 1) const;

    virtual void printUsageInfo() const;

    size_t nextFreeBlock(size_t size, size_t alignment, size_t start) const;


    class AllocBlockIterator
    {
    public:
        using iterator_category = eastl::input_iterator_tag;
        using value_type        = size_t;
        using pointer           = value_type*;
        using const_pointer     = const value_type*;
        using reference         = value_type&;
        using const_reference   = const value_type&;

        AllocBlockIterator(const BootstrapRangeAllocator* allocator, size_t size, size_t alignment, size_t start) :
            allocator_(allocator), size_(size), alignment_(alignment), curr_(start) {}

        const_reference operator*() const { return curr_;  }
        const_pointer operator->()  const { return &curr_; }

        AllocBlockIterator& operator++() { curr_ = allocator_->nextFreeBlock(size_, alignment_, curr_ + size_); return *this; }

        friend bool operator== (const AllocBlockIterator& a, const AllocBlockIterator& b) { return a.curr_ == b.curr_; };
        friend bool operator!= (const AllocBlockIterator& a, const AllocBlockIterator& b) { return a.curr_ != b.curr_; };

    private:
        const BootstrapRangeAllocator* allocator_;

        size_t size_;
        size_t alignment_;
        size_t curr_;
    };

    AllocBlockIterator freeBlocksBegin(size_t size, size_t alignment) const { return AllocBlockIterator(this, size, alignment, nextFreeBlock(size, alignment, 0)); }
    AllocBlockIterator freeBlocksEnd(size_t size, size_t alignment)   const { return AllocBlockIterator(this, size, alignment, -1); }


    class FreeBlocks
    {
    public:
        FreeBlocks(const BootstrapRangeAllocator* allocator, size_t size, size_t alignment) :
            allocator_(allocator), size_(size), alignment_(alignment) {}

        AllocBlockIterator begin() const { return allocator_->freeBlocksBegin(size_, alignment_); }
        AllocBlockIterator end()   const { return allocator_->freeBlocksEnd(size_, alignment_);   }

    private:
        const BootstrapRangeAllocator* allocator_;
        size_t size_;
        size_t alignment_;
    };

    FreeBlocks freeBlocks(size_t size, size_t alignment) const { return FreeBlocks(this, size, alignment); }

protected:

    IntervalSet<size_t, eastl::fixed_allocator> iset_;
    decltype(iset_)::node_type iset_buffer_[10];
};
