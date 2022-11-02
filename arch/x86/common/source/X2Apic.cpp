#include "X2Apic.h"
#include "CPUID.h"
#include "MSR.h"

bool X2Apic::x2ApicSupported()
{
    CpuFeatures f;
    return f.cpuHasFeature(CpuFeatures::X86Feature::X2APIC);
}

bool X2Apic::isEnabled()
{
    auto msr = Register::APIC_BASE::read();
    return msr.enable && msr.x2apic_enable;
}

void X2Apic::enableX2ApicMode()
{
    auto msr = Register::APIC_BASE::read();
    msr.x2apic_enable = 1;
    Register::APIC_BASE::write(msr);
}

uint32_t X2Apic::readId()
{
    return Register::ID::read();
}
