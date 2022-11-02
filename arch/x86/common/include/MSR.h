#pragma once

#include <cstdint>
#include "types.h"
#include "EASTL/type_traits.h"

#define MSR_FS_BASE        0xC0000100
#define MSR_GS_BASE        0xC0000101
#define MSR_KERNEL_GS_BASE 0xC0000102


void getMSR(uint32 msr, uint32 *lo, uint32 *hi);
void setMSR(uint32 msr, uint32 lo, uint32 hi);

template<uint32_t msr, typename RegT, bool readable = true, bool writeable = true>
class MSR
{
public:
    static constexpr uint32_t MSR_ID = msr;
    using value_type = RegT;

    static_assert(sizeof(value_type) == sizeof(uint32_t) || sizeof(value_type) == sizeof(uint64_t));

    template <uint32_t _msr = msr, bool _readable = readable>
    static eastl::enable_if_t<_readable, value_type> read()
    {
        value_type val;
        if constexpr (sizeof(value_type) == 8)
        {
            getMSR(static_cast<uint32_t>(msr), (uint32_t*)&val, ((uint32_t*)&val) + 1);
        }
        else
        {
            uint32_t ignored;
            getMSR(static_cast<uint32_t>(msr), (uint32_t*)&val, &ignored);
        }
        return val;
    }

    template <uint32_t _msr = msr, bool _writeable = writeable>
    static eastl::enable_if_t<_writeable, void> write(value_type val)
    {
        if constexpr (sizeof(value_type) == 8)
        {
            setMSR(static_cast<uint32_t>(msr), *(uint32_t*)&val, *(((uint32_t*)&val) + 1));
        }
        else
        {
            uint32_t ignored = 0;
            setMSR(static_cast<uint32_t>(msr), *(uint32_t*)&val, &ignored);
        }
    }
private:
};
