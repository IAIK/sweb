#pragma once

#include "RangeAllocator.h"

#include <EASTL/fixed_allocator.h>

/* This BootstrapRangeAllocator is 'good enough' for temporary use during initialization
 * of the page manager, but should not really be used elsewhere
 */
class BootstrapRangeAllocator : public RangeAllocator<eastl::fixed_allocator>
{
public:
    BootstrapRangeAllocator();
    BootstrapRangeAllocator(const BootstrapRangeAllocator&) = delete;
    ~BootstrapRangeAllocator() override = default;

private:
    // Fixed buffer for 10 elements
    decltype(iset_)::node_type iset_buffer_[10];
};
