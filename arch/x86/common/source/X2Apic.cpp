#include "X2Apic.h"
#include "CPUID.h"
#include "MSR.h"
#include "ArchInterrupts.h"
#include "debug.h"

cpu_local X2Apic cpu_x2apic_impl;

bool X2Apic::x2ApicSupported()
{
    return cpu_features.cpuHasFeature(CpuFeatures::X86Feature::X2APIC);
}

void X2Apic::enableX2ApicMode()
{
    debug(APIC, "Enabling x2APIC mode\n");
    assert(x2ApicSupported());
    auto msr = MSR::IA32_APIC_BASE::read();
    assert(msr.enable);
    msr.x2apic_enable = 1;
    MSR::IA32_APIC_BASE::write(msr);
    cpu_lapic = &cpu_x2apic_impl;
}

bool X2Apic::isEnabled()
{
    auto msr = MSR::IA32_APIC_BASE::read();
    debug(APIC, "x2apic enabled: %u, apic enabled: %u\n", msr.x2apic_enable, msr.enable);
    return msr.enable && msr.x2apic_enable;
}

void X2Apic::writeRegisterImpl(ApicRegisterOffset offset, uint64_t v)
{
    if (offset != ApicRegisterOffset::EOI)
        debug(APIC, "Write register %x, %lx\n", (unsigned int)offset, v);
    uint32 vl = v;
    uint32 vh = v >> 32;
    MSR::setMSR(x2ApicOffset2Msr(offset), vl, vh);
    if (offset != ApicRegisterOffset::EOI)
        debug(APIC, "Register write complete\n");
}

uint64_t X2Apic::readRegisterImpl(ApicRegisterOffset offset)
{
    assert(offset != ApicRegisterOffset::INTERRUPT_COMMAND_H);
    uint64_t v = 0;
    MSR::getMSR(x2ApicOffset2Msr(offset), (uint32_t*)&v, ((uint32_t*)&v)+1);
    return v;
}

void X2Apic::writeIcr(InterruptCommandRegisterLow icr_l, uint32_t dest)
{
    InterruptCommandRegister icr{};
    icr.h.x2apic_destination = dest;
    icr.l = icr_l;

    writeRegister<Register::INTERRUPT_COMMAND>(icr);
}

void X2Apic::init()
{
    debug(APIC, "Initializing Local x2APIC\n");
    assert(!initialized_);
    assert(isEnabled());

    id_ = readId();
    auto logical_dest_id = readRegister<Register::LOGICAL_DESTINATION>();
    debug(APIC, "Local x2APIC, id: %x, logical dest: %x\n", Id(), logical_dest_id);

    setErrorInterruptVector(ERROR_INTERRUPT_VECTOR);
    setSpuriousInterruptNumber(100);
    initTimer();
    enable(true);

    initialized_ = true;
    SMP::currentCpu().setId(id_);
}

uint32_t X2Apic::readId()
{
    return X2ApicRegisters::ID::read().x2apic_id;
}
