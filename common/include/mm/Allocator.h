#pragma once
#include "types.h"
#include "paging-definitions.h"
#include "Bitmap.h"
#include "assert.h"
#include "debug.h"

class Allocator
{
public:
        virtual size_t alloc(size_t size, size_t alignment = 1) = 0;
        virtual bool dealloc(size_t start, size_t size) = 0;

        virtual void setUseable(size_t start, size_t end) = 0;
        virtual void setUnuseable(size_t start, size_t end) = 0;

        virtual size_t numFree() = 0;
        virtual size_t numFreeContiguousBlocks(size_t size, size_t alignment = 1) = 0;

        virtual void printUsageInfo() = 0;
private:
};

/* This BootstrapRangeAllocator is 'good enough' for temporary use during initialization
 * of the page manager, but should not really be used elsewhere
 */
class BootstrapRangeAllocator : public Allocator
{
public:
        BootstrapRangeAllocator() = default;
        virtual ~BootstrapRangeAllocator() = default;

        virtual size_t alloc(size_t size, size_t alignment = 1);
        virtual bool dealloc(size_t start, size_t size);

        virtual void setUseable(size_t start, size_t end);
        virtual void setUnuseable(size_t start, size_t end);

        virtual size_t numFree();
        virtual size_t numFreeContiguousBlocks(size_t size, size_t alignment = 1);

        virtual void printUsageInfo();

        struct UseableRange
        {
                size_t start;
                size_t end;
        };


private:
        UseableRange useable_ranges_[20];

        bool slotIsUsed(size_t i);
        ssize_t findFirstFreeSlot();
};

template<size_t BLOCK_SIZE=PAGE_SIZE>
class BitmapAllocator : public Allocator
{
public:
        BitmapAllocator(size_t size) :
                bitmap_(size/BLOCK_SIZE)
        {
                assert(size % BLOCK_SIZE == 0);
        }

        BitmapAllocator(size_t size, Allocator&& alloc_template) :
            bitmap_(size/BLOCK_SIZE)
        {
            assert(size % BLOCK_SIZE == 0);

            setUnuseable(0, size);
            alloc_template.printUsageInfo();
            debug(PM, "Num free pages: %zu\n", alloc_template.numFreeContiguousBlocks(PAGE_SIZE, PAGE_SIZE));
            size_t free_phys_page;
            while((free_phys_page = alloc_template.alloc(PAGE_SIZE, PAGE_SIZE)) != (size_t)-1)
            {
                setUseable(free_phys_page, free_phys_page + PAGE_SIZE);
            }
        }

        virtual ~BitmapAllocator() = default;

        virtual size_t alloc(size_t size = BLOCK_SIZE, size_t alignment = BLOCK_SIZE)
        {
                assert(size % BLOCK_SIZE == 0);
                assert(alignment && (alignment % BLOCK_SIZE == 0));

                size_t aligned_start = lowest_unreserved_address_;
                aligned_start += (aligned_start % alignment ? alignment - aligned_start % alignment : 0);
                for(size_t i = aligned_start; i + size <= bitmap_.getSize()*BLOCK_SIZE; i += alignment)
                {
                        if(isRangeFree(i, size))
                        {
                                setUnuseable(i, i + size);
                                return i;
                        }
                }

                return -1;
        }


        virtual bool dealloc(size_t start, size_t size = BLOCK_SIZE)
        {
                assert(start % BLOCK_SIZE == 0);
                assert(size % BLOCK_SIZE == 0);

                for(size_t i = start; i < (start + size); i += BLOCK_SIZE)
                {
                        if(!bitmap_.getBit(i/BLOCK_SIZE))
                        {
                                return false;
                        }
                }

                setUseable(start, start + size);
                return true;
        }


        virtual void setUseable(size_t start, size_t end)
        {
                assert(start % BLOCK_SIZE == 0);
                assert(end % BLOCK_SIZE == 0);

                for(size_t i = start/BLOCK_SIZE; i < end/BLOCK_SIZE; ++i)
                {
                        bitmap_.unsetBit(i);
                }

                if(start < lowest_unreserved_address_)
                {
                        lowest_unreserved_address_ = start;
                }
        }


        virtual void setUnuseable(size_t start, size_t end)
        {
                assert(start % BLOCK_SIZE == 0);
                assert(end % BLOCK_SIZE == 0);

                for(size_t i = start/BLOCK_SIZE; i < end/BLOCK_SIZE; ++i)
                {
                        bitmap_.setBit(i);
                }

                while((lowest_unreserved_address_ < bitmap_.getSize()*BLOCK_SIZE) && bitmap_.getBit(lowest_unreserved_address_/BLOCK_SIZE))
                {
                        lowest_unreserved_address_ += BLOCK_SIZE;
                }
        }

        virtual size_t numFree()
        {
                return bitmap_.getNumFreeBits() * BLOCK_SIZE;
        }

        virtual size_t numFreeContiguousBlocks(size_t size = BLOCK_SIZE, size_t alignment = BLOCK_SIZE)
        {
            assert(size % BLOCK_SIZE == 0);
            assert(alignment && (alignment % BLOCK_SIZE == 0));

            return bitmap_.getNumFreeBits();
        }

        virtual void printUsageInfo()
        {
                bitmap_.bmprint();
        }

private:
        bool isRangeFree(size_t start, size_t size)
        {
                assert(start % BLOCK_SIZE == 0);
                assert(size % BLOCK_SIZE == 0);

                for(size_t i = start/BLOCK_SIZE; i < (start + size)/BLOCK_SIZE; ++i)
                {
                        if(bitmap_.getBit(i))
                                return false;
                }

                return true;
        }

        Bitmap bitmap_;
        size_t lowest_unreserved_address_;
};
