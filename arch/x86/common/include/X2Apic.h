#pragma once

#include <cstdint>
#include "MSR.h"
#include "APIC.h"


struct IA32_APIC_BASE
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
static_assert(sizeof(IA32_APIC_BASE) == 8);




namespace X2Apic
{
    bool x2ApicSupported();
    void enableX2ApicMode();
    bool isEnabled();
    uint32_t readId();

    enum X2APIC_MSR
    {
        IA32_APIC_BASE = 0x1B,
        ID = 0x802,
        VERSION = 0x803,
        TASK_PRIORITY = 0x808,
        PROCESSOR_PRIORITY = 0x80A,
        EOI = 0x80B,
        LOGICAL_DESTINATION = 0x80D,
        SPURIOUS_INTERRUPT_VECTOR = 0x80F,
        ISR_31_0    = 0x810,
        ISR_63_32   = 0x811,
        ISR_95_64   = 0x812,
        ISR_127_96  = 0x813,
        ISR_159_128 = 0x814,
        ISR_191_160 = 0x815,
        ISR_223_192 = 0x816,
        ISR_255_224 = 0x817,
        TMR_31_0    = 0x818,
        TMR_63_32   = 0x819,
        TMR_95_64   = 0x81A,
        TMR_127_96  = 0x81B,
        TMR_159_128 = 0x81C,
        TMR_191_160 = 0x81D,
        TMR_223_192 = 0x81E,
        TMR_255_224 = 0x81F,
        IRR_31_0    = 0x820,
        IRR_63_32   = 0x821,
        IRR_95_64   = 0x822,
        IRR_127_96  = 0x823,
        IRR_159_128 = 0x824,
        IRR_191_160 = 0x825,
        IRR_223_192 = 0x826,
        IRR_255_224 = 0x827,
        ERROR_STATUS = 0x828,
        LVT_CMCI = 0x82F,
        INTERRUPT_COMMAND = 0x830,
        LVT_TIMER = 0x832,
        LVT_THERMAL_SENSOR = 0x833,
        LVT_PERFMON = 0x834,
        LVT_LINT0 = 0x835,
        LVT_LINT1 = 0x836,
        LVT_ERROR = 0x837,
        TIMER_INITIAL_COUNT = 0x838,
        TIMER_CURRENT_COUNT = 0x839,
        TIMER_DIVIDE_CONFIG = 0x83E,
        SELF_IPI = 0x83F,
    };

    namespace Register
    {
        using APIC_BASE = MSR<X2APIC_MSR::IA32_APIC_BASE, struct IA32_APIC_BASE, true, true>;
        using ID = MSR<X2APIC_MSR::ID, uint32_t, true, false>;
        using VERSION = MSR<X2APIC_MSR::VERSION, LocalAPIC_VersionRegister, true, false>;
        using TASK_PRIORITY = MSR<X2APIC_MSR::TASK_PRIORITY, LocalAPIC_PriorityRegister, true, true>;
        using PROCESSOR_PRIORITY = MSR<X2APIC_MSR::PROCESSOR_PRIORITY, LocalAPIC_PriorityRegister, true, false>;
        using EOI = MSR<X2APIC_MSR::EOI, uint32_t, false, true>;
        using LOGICAL_DESTINATION = MSR<X2APIC_MSR::LOGICAL_DESTINATION, uint32_t, true, false>;
        using SPURIOUS_INTERRUPT_VECTOR = MSR<X2APIC_MSR::SPURIOUS_INTERRUPT_VECTOR, LocalAPIC_SpuriousInterruptVector, true, true>;
        using ISR_31_0 = MSR<X2APIC_MSR::ISR_31_0, uint32_t, true, false>;
        using ISR_63_32 = MSR<X2APIC_MSR::ISR_63_32, uint32_t, true, false>;
        using ISR_95_64 = MSR<X2APIC_MSR::ISR_95_64, uint32_t, true, false>;
        using ISR_127_96 = MSR<X2APIC_MSR::ISR_127_96, uint32_t, true, false>;
        using ISR_159_128 = MSR<X2APIC_MSR::ISR_159_128, uint32_t, true, false>;
        using ISR_191_160 = MSR<X2APIC_MSR::ISR_191_160, uint32_t, true, false>;
        using ISR_223_192 = MSR<X2APIC_MSR::ISR_223_192, uint32_t, true, false>;
        using ISR_255_224 = MSR<X2APIC_MSR::ISR_255_224, uint32_t, true, false>;
        using TMR_31_0 = MSR<X2APIC_MSR::TMR_31_0, uint32_t, true, false>;
        using TMR_63_32 = MSR<X2APIC_MSR::TMR_63_32, uint32_t, true, false>;
        using TMR_95_64 = MSR<X2APIC_MSR::TMR_95_64, uint32_t, true, false>;
        using TMR_127_96 = MSR<X2APIC_MSR::TMR_127_96, uint32_t, true, false>;
        using TMR_159_128 = MSR<X2APIC_MSR::TMR_159_128, uint32_t, true, false>;
        using TMR_191_160 = MSR<X2APIC_MSR::TMR_191_160, uint32_t, true, false>;
        using TMR_223_192 = MSR<X2APIC_MSR::TMR_223_192, uint32_t, true, false>;
        using TMR_255_224 = MSR<X2APIC_MSR::TMR_255_224, uint32_t, true, false>;
        using IRR_31_0 = MSR<X2APIC_MSR::IRR_31_0, uint32_t, true, false>;
        using IRR_63_32 = MSR<X2APIC_MSR::IRR_63_32, uint32_t, true, false>;
        using IRR_95_64 = MSR<X2APIC_MSR::IRR_95_64, uint32_t, true, false>;
        using IRR_127_96 = MSR<X2APIC_MSR::IRR_127_96, uint32_t, true, false>;
        using IRR_159_128 = MSR<X2APIC_MSR::IRR_159_128, uint32_t, true, false>;
        using IRR_191_160 = MSR<X2APIC_MSR::IRR_191_160, uint32_t, true, false>;
        using IRR_223_192 = MSR<X2APIC_MSR::IRR_223_192, uint32_t, true, false>;
        using IRR_255_224 = MSR<X2APIC_MSR::IRR_255_224, uint32_t, true, false>;
        using ERROR_STATUS = MSR<X2APIC_MSR::ERROR_STATUS, LocalAPIC_ErrorStatusRegister, true, true>;
        using LVT_CMCI = MSR<X2APIC_MSR::LVT_CMCI, uint32_t, true, true>;
        using INTERRUPT_COMMAND = MSR<X2APIC_MSR::INTERRUPT_COMMAND, uint64_t, true, true>;
        using LVT_TIMER = MSR<X2APIC_MSR::LVT_TIMER, LocalAPIC_LVT_TimerRegister, true, true>;
        using LVT_THERMAL_SENSOR = MSR<X2APIC_MSR::LVT_THERMAL_SENSOR, uint32_t, true, true>;
        using LVT_PERFMON = MSR<X2APIC_MSR::LVT_PERFMON, uint32_t, true, true>;
        using LVT_LINT0 = MSR<X2APIC_MSR::LVT_LINT0, LocalAPIC_LVT_LINTRegister, true, true>;
        using LVT_LINT1 = MSR<X2APIC_MSR::LVT_LINT1, LocalAPIC_LVT_LINTRegister, true, true>;
        using LVT_ERROR = MSR<X2APIC_MSR::LVT_ERROR, LocalAPIC_LVT_ErrorRegister, true, true>;
        using TIMER_INITIAL_COUNT = MSR<X2APIC_MSR::TIMER_INITIAL_COUNT, uint32_t, true, true>;
        using TIMER_CURRENT_COUNT = MSR<X2APIC_MSR::TIMER_CURRENT_COUNT, uint32_t, true, false>;
        using TIMER_DIVIDE_CONFIG = MSR<X2APIC_MSR::TIMER_DIVIDE_CONFIG, LocalAPIC_TimerDivideConfigRegister, true, true>;
        using SELF_IPI = MSR<X2APIC_MSR::SELF_IPI, LocalAPIC_LVT_ErrorRegister, false, true>;
    };
};
