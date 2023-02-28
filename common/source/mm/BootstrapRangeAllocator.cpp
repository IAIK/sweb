#include "BootstrapRangeAllocator.h"

BootstrapRangeAllocator::BootstrapRangeAllocator()
{
    iset_.get_allocator().init(iset_buffer_,
                               sizeof(iset_buffer_),
                               sizeof(decltype(iset_)::node_type),
                               alignof(decltype(iset_)::node_type));
}
