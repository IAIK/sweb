#pragma once

#include <cstdint>
#include "types.h"
#include "EASTL/type_traits.h"

namespace MSR
{

#define MSR_FS_BASE        0xC0000100
#define MSR_GS_BASE        0xC0000101
#define MSR_KERNEL_GS_BASE 0xC0000102
#define MSR_IA32_APIC_BASE 0x1B


    void getMSR(uint32_t msr, uint32_t* lo, uint32_t* hi);
    void setMSR(uint32_t msr, uint32_t lo, uint32_t hi);

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
                getMSR(msr, (uint32_t*)&val, ((uint32_t*)&val) + 1);
            }
            else
            {
                uint32_t ignored;
                getMSR(msr, (uint32_t*)&val, &ignored);
            }
            return val;
        }

        template <uint32_t _msr = msr, bool _writeable = writeable>
        static eastl::enable_if_t<_writeable, void> write(value_type val)
        {
            if constexpr (sizeof(value_type) == 8)
            {
                setMSR(msr, *(uint32_t*)&val, *(((uint32_t*)&val) + 1));
            }
            else
            {
                setMSR(msr, *(uint32_t*)&val, 0);
            }
        }
    private:
    };

    struct IA32_APIC_BASE_t
    {
        union
        {
            struct
            {
                uint64_t reserved1     :  8;
                uint64_t bsp           :  1;
                uint64_t reserved2     :  1;
                uint64_t x2apic_enable :  1;
                uint64_t enable        :  1;
                uint64_t apic_base     : 24;
                uint64_t reserved3     : 28;
            };
            struct
            {
                uint32_t value32_l;
                uint32_t value32_h;
            };
            uint64_t value64;
        };
    };
    static_assert(sizeof(IA32_APIC_BASE_t) == 8);


    using IA32_APIC_BASE = MSR<MSR_IA32_APIC_BASE, struct IA32_APIC_BASE_t, true, true>;
};
