#pragma once

#include "APIC.h"
#include "CPUID.h"
#include "MSR.h"
#include <cstdint>
#include "debug.h"

class X2Apic : public Apic
{
public:
    X2Apic() :
        Apic(eastl::string("x2APIC ") + eastl::to_string(CPUID::localX2ApicId()))
    {
    }
    ~X2Apic() override = default;

    static bool x2ApicSupported();
    static void enableX2ApicMode();
    static bool isEnabled();

    void init() override;
    uint32_t readId() override;

    bool isX2Apic() override
    {
        return true;
    }

private:
    constexpr static unsigned int x2ApicOffset2Msr(ApicRegisterOffset reg)
    {
        return 0x800 + (static_cast<unsigned int>(reg) >> 4);
    }

    struct X2ApicRegisters
    {
        template<typename apic_reg>
        struct X2ApicRegister : public MSR::MSR<x2ApicOffset2Msr(apic_reg::reg_offset), typename apic_reg::value_type, apic_reg::readable, apic_reg::writeable>{};

        using ID = X2ApicRegister<Register::ID>;
        using VERSION = X2ApicRegister<Register::VERSION>;
        using TASK_PRIORITY = X2ApicRegister<Register::TASK_PRIORITY>;
        using PROCESSOR_PRIORITY = X2ApicRegister<Register::PROCESSOR_PRIORITY>;
        using EOI = X2ApicRegister<Register::EOI>;
        using LOGICAL_DESTINATION = X2ApicRegister<Register::LOGICAL_DESTINATION>;
        using SPURIOUS_INTERRUPT_VECTOR = X2ApicRegister<Register::SPURIOUS_INTERRUPT_VECTOR>;
        using ISR_31_0 = X2ApicRegister<Register::ISR_31_0>;
        using ISR_63_32 = X2ApicRegister<Register::ISR_63_32>;
        using ISR_95_64 = X2ApicRegister<Register::ISR_95_64>;
        using ISR_127_96 = X2ApicRegister<Register::ISR_127_96>;
        using ISR_159_128 = X2ApicRegister<Register::ISR_159_128>;
        using ISR_191_160 = X2ApicRegister<Register::ISR_191_160>;
        using ISR_223_192 = X2ApicRegister<Register::ISR_223_192>;
        using ISR_255_224 = X2ApicRegister<Register::ISR_255_224>;
        using TMR_31_0 = X2ApicRegister<Register::TMR_31_0>;
        using TMR_63_32 = X2ApicRegister<Register::TMR_63_32>;
        using TMR_95_64 = X2ApicRegister<Register::TMR_95_64>;
        using TMR_127_96 = X2ApicRegister<Register::TMR_127_96>;
        using TMR_159_128 = X2ApicRegister<Register::TMR_159_128>;
        using TMR_191_160 = X2ApicRegister<Register::TMR_191_160>;
        using TMR_223_192 = X2ApicRegister<Register::TMR_223_192>;
        using TMR_255_224 = X2ApicRegister<Register::TMR_255_224>;
        using IRR_31_0 = X2ApicRegister<Register::IRR_31_0>;
        using IRR_63_32 = X2ApicRegister<Register::IRR_63_32>;
        using IRR_95_64 = X2ApicRegister<Register::IRR_95_64>;
        using IRR_127_96 = X2ApicRegister<Register::IRR_127_96>;
        using IRR_159_128 = X2ApicRegister<Register::IRR_159_128>;
        using IRR_191_160 = X2ApicRegister<Register::IRR_191_160>;
        using IRR_223_192 = X2ApicRegister<Register::IRR_223_192>;
        using IRR_255_224 = X2ApicRegister<Register::IRR_255_224>;
        using ERROR_STATUS = X2ApicRegister<Register::ERROR_STATUS>;
        using LVT_CMCI = X2ApicRegister<Register::LVT_CMCI>;
        using INTERRUPT_COMMAND = X2ApicRegister<Register::INTERRUPT_COMMAND>;
        using LVT_TIMER = X2ApicRegister<Register::LVT_TIMER>;
        using LVT_THERMAL_SENSOR = X2ApicRegister<Register::LVT_THERMAL_SENSOR>;
        using LVT_PERFMON = X2ApicRegister<Register::LVT_PERFMON>;
        using LVT_LINT0 = X2ApicRegister<Register::LVT_LINT0>;
        using LVT_LINT1 = X2ApicRegister<Register::LVT_LINT1>;
        using LVT_ERROR = X2ApicRegister<Register::LVT_ERROR>;
        using TIMER_INITIAL_COUNT = X2ApicRegister<Register::TIMER_INITIAL_COUNT>;
        using TIMER_CURRENT_COUNT = X2ApicRegister<Register::TIMER_CURRENT_COUNT>;
        using TIMER_DIVIDE_CONFIG = X2ApicRegister<Register::TIMER_DIVIDE_CONFIG>;
        using SELF_IPI = X2ApicRegister<Register::SELF_IPI>;
    };

protected:
    void writeRegisterImpl(ApicRegisterOffset offset, uint64_t v) override;
    uint64_t readRegisterImpl(ApicRegisterOffset offset) override;

    void writeIcr(InterruptCommandRegisterLow icr_l, uint32_t dest) override;

    void waitIpiDelivered() override {} // Do nothing since delivered bit was removed for x2apic

private:
};
