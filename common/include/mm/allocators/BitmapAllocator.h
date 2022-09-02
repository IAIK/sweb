#pragma once
#include "Allocator.h"

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
            debug(PM, "Num free pages: %zu\n", alloc_template.numFreeBlocks(PAGE_SIZE, PAGE_SIZE));
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

        virtual bool allocTarget([[maybe_unused]]size_t start, [[maybe_unused]]size_t size)
        {
            // TODO
            return false;
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

        virtual size_t numFree() const
        {
                return bitmap_.getNumFreeBits() * BLOCK_SIZE;
        }

        virtual size_t numFreeBlocks(size_t size = BLOCK_SIZE, size_t alignment = BLOCK_SIZE) const
        {
            assert(size % BLOCK_SIZE == 0);
            assert(alignment && (alignment % BLOCK_SIZE == 0));

            return bitmap_.getNumFreeBits();
        }

        virtual void printUsageInfo() const
        {
                bitmap_.bmprint();
        }

private:
        bool isRangeFree(size_t start, size_t size) const
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
