#pragma once
#include "types.h"
#include "paging-definitions.h"
#include "Bitmap.h"
#include "assert.h"
#include "debug.h"
#include "EASTL/iterator.h"

struct HalfOpenInterval
{
    size_t start;
    size_t end;

    bool contains(size_t addr)
    {
        return start <= addr && addr < end;
    }

    enum IntervalRelation
    {

    };
};

class Allocator
{
public:
        virtual size_t alloc(size_t size, size_t alignment = 1) = 0;
        virtual bool dealloc(size_t start, size_t size) = 0;

        virtual void setUseable(size_t start, size_t end) = 0;
        virtual void setUnuseable(size_t start, size_t end) = 0;

        virtual size_t numFree() const = 0;
        virtual size_t numFreeBlocks(size_t size, size_t alignment = 1) const = 0;

        virtual void printUsageInfo() const = 0;
private:
};