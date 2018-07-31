#pragma once

#include "types.h"

struct CoreLocalStorage
{
        CoreLocalStorage* cls_ptr;
        size_t core_id;
        SegmentDescriptor gdt[7];
        TSS tss;
};

void setCLS(CoreLocalStorage* cls);

CoreLocalStorage* getCLS();

CoreLocalStorage* initCLS();


size_t getCoreID();
