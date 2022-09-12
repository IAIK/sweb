#pragma once

#include "EASTL/atomic.h"
#include "EASTL/fixed_function.h"
#include <cstddef>

struct RemoteFunctionCallMessage
{
    using function_t = eastl::fixed_function<8, void (void)>;

    function_t func;

    eastl::atomic<bool> received;
    eastl::atomic<bool> done;

    size_t target_cpu;
    size_t orig_cpu;

    eastl::atomic<RemoteFunctionCallMessage*> next;
};
