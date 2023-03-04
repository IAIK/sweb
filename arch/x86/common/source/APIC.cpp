#include "APIC.h"
#include "debug.h"
#include "assert.h"
#include "ArchMemory.h"
#include "PageManager.h"
#include "new.h"
#include "kstring.h"
#include "InterruptUtils.h"
#include "ArchInterrupts.h"
#include "Scheduler.h"
#include "ArchMulticore.h"
#include "ProgrammableIntervalTimer.h"
#include "8259.h"
#include "CPUID.h"
#include "offsets.h"
#include "Device.h"
#include "IrqDomain.h"
#include "MSR.h"
#include "X2Apic.h"
#include "ports.h"
#include "ArchCommon.h"


void* XApic::reg_paddr_ = (void*)0xfee00000;
void* XApic::reg_vaddr_ = (void*)0;

eastl::vector<MADTProcLocalAPIC> Apic::local_apic_list_{};

extern volatile size_t outstanding_EOIs;

Apic::Apic(const eastl::string& name) :
    IrqDomain(name, 256, this),
    Device(name),
    timer_interrupt_controller(*this),
    inter_processor_interrupt_domain("Inter Processor Interrupt", 256)
{
}

uint32 Apic::apicId() const
{
    return id_;
}

bool Apic::isInitialized() const { return initialized_; }

bool Apic::isX2Apic() { return false; }

void Apic::setIMCRMode(IMCRData mode)
{
    // Intel MultiProcessor Specification chapter 3.6.2
    // https://pdos.csail.mit.edu/6.828/2008/readings/ia32/MPspec.pdf
    debug(APIC, "Ensure IMCR is set to APIC passthrough/symmetric mode\n");
    // IMCR register might not actually exist, but attempting to write to it should be fine?
    IMCR_SELECT::write(IMCRSelect::SELECT_IMCR);
    IMCR_DATA::write(mode);
}

void Apic::globalEnable(bool enable)
{
    auto apic_base = MSR::IA32_APIC_BASE::read();
    apic_base.enable = enable;
    MSR::IA32_APIC_BASE::write(apic_base);
}

void Apic::enable(bool enable)
{
    debug(APIC, "%s APIC %x\n", (enable ? "Enabling" : "Disabling"), apicId());

    WithInterrupts i(false);
    auto siv = readRegister<Register::SPURIOUS_INTERRUPT_VECTOR>();
    siv.enable = enable;
    writeRegister<Register::SPURIOUS_INTERRUPT_VECTOR>(siv);
}

void Apic::initTimer()
{
    debug(APIC, "Init timer for APIC %x\n", apicId());
    assert(!ArchInterrupts::testIFSet());

    setUsingAPICTimer(true);

    auto div = readRegister<Register::TIMER_DIVIDE_CONFIG>();
    div.setTimerDivisor(TIMER_DIVISOR);
    writeRegister<Register::TIMER_DIVIDE_CONFIG>(div);

    auto timer_reg = readRegister<Register::LVT_TIMER>();
    timer_reg.setVector(APIC_TIMER_VECTOR);
    timer_reg.setMode(TimerMode::PERIODIC);
    timer_reg.setMask(true);
    writeRegister<Register::LVT_TIMER>(timer_reg);

    // Write to initial count register starts timer
    setTimerPeriod(0x500000);

}

void Apic::setTimerPeriod(uint32 count)
{
    debug(APIC, "Set timer period %x\n", count);
    writeRegister<Apic::Register::TIMER_INITIAL_COUNT>(count);
}

void Apic::setSpuriousInterruptNumber(uint8 num)
{
    auto siv = readRegister<Apic::Register::SPURIOUS_INTERRUPT_VECTOR>();
    siv.vector = num;
    writeRegister<Apic::Register::SPURIOUS_INTERRUPT_VECTOR>(siv);
}

void Apic::setErrorInterruptVector(uint8_t vector)
{
    LVT_ErrorRegister errorreg{};
    errorreg.vector = vector;
    errorreg.mask = false;
    writeRegister<Register::LVT_ERROR>(errorreg);
}

void Apic::setUsingAPICTimer(bool use_apic_timer)
{
    debug(APIC, "Set using APIC timer: %d\n", use_apic_timer);

    use_apic_timer_ = use_apic_timer;
}

bool Apic::usingAPICTimer() const
{
    return use_apic_timer_;
}

bool Apic::checkISR(uint8_t irqnum)
{
    uint8 word_offset = irqnum/32;
    uint8 bit_offset = irqnum % 32;
    assert(word_offset < 8);

    uint32_t isr = 0;

    switch (word_offset)
    {
    case 0:
        isr = readRegister<Register::ISR_31_0>();
        break;
    case 1:
        isr = readRegister<Register::ISR_63_32>();
        break;
    case 2:
        isr = readRegister<Register::ISR_95_64>();
        break;
    case 3:
        isr = readRegister<Register::ISR_127_96>();
        break;
    case 4:
        isr = readRegister<Register::ISR_159_128>();
        break;
    case 5:
        isr = readRegister<Register::ISR_191_160>();
        break;
    case 6:
        isr = readRegister<Register::ISR_223_192>();
        break;
    case 7:
        isr = readRegister<Register::ISR_255_224>();
        break;
    default:
        assert(!"Invalid ISR word offset");
    }
    return isr & (1 << bit_offset);
}

bool Apic::checkIRR(uint8_t irqnum)
{
    uint8 word_offset = irqnum/32;
    uint8 bit_offset = irqnum % 32;
    assert(word_offset < 8);

    uint32_t irr = 0;

    switch (word_offset)
    {
    case 0:
        irr = readRegister<Register::IRR_31_0>();
        break;
    case 1:
        irr = readRegister<Register::IRR_63_32>();
        break;
    case 2:
        irr = readRegister<Register::IRR_95_64>();
        break;
    case 3:
        irr = readRegister<Register::IRR_127_96>();
        break;
    case 4:
        irr = readRegister<Register::IRR_159_128>();
        break;
    case 5:
        irr = readRegister<Register::IRR_191_160>();
        break;
    case 6:
        irr = readRegister<Register::IRR_223_192>();
        break;
    case 7:
        irr = readRegister<Register::IRR_255_224>();
        break;
    default:
        assert(!"Invalid IRR word offset");
    }
    return irr & (1 << bit_offset);
}

void Apic::sendEOI(size_t num)
{
    assert(SMP::currentCpuId() == apicId());
    --outstanding_EOIs_;
    if(APIC & OUTPUT_ADVANCED)
    {
        debug(APIC, "CPU %zu, Sending EOI for %zx\n", SMP::currentCpuId(), num);
        for(size_t i = 0; i < 256; ++i)
        {
            if(checkISR(i))
            {
                debug(APIC, "CPU %zx, interrupt %zx being serviced\n", SMP::currentCpuId(), i);
            }
        }
        for(size_t i = 0; i < 256; ++i)
        {
            if(checkIRR(i))
            {
                debug(APIC, "CPU %zx, interrupt %zx pending\n", SMP::currentCpuId(), i);
            }
        }
    }

    assert(!ArchInterrupts::testIFSet() && "Attempted to send end of interrupt command while interrupts are enabled");
    assert(checkISR(num) && "Attempted to send end of interrupt command but interrupt is not actually being serviced");

    writeRegister<Register::EOI>(0);
}

bool Apic::mask(irqnum_t irq, bool mask)
{
    // Cannot mask interrupts in APIC
    debug(APIC, "%s, mask Irq %zx = %u\n", name().c_str(), irq, mask);
    auto info = irqInfo(irq);
    assert(info && "No irq info found");
    assert(!info->mapped_by.empty());

    return false;
}

bool Apic::isMasked([[maybe_unused]]irqnum_t irq)
{
    // Cannot mask interrupts in APIC
    return false;
}

bool Apic::ack(irqnum_t irq)
{
    debugAdvanced(APIC, "%s, ack Irq %zx\n", name().c_str(), irq);
    auto info = irqInfo(irq);
    assert(info && "No irq info found");
    assert(!info->mapped_by.empty());

    return false;
}

bool Apic::irqStart(irqnum_t irq)
{
    debugAdvanced(APIC, "%s, start of Irq %zx\n", name().c_str(), irq);
    auto info = irqInfo(irq);
    if (!info)
        debugAlways(APIC, "No irq info found for irqnum %zu in irq domain %s\n", irq, name().c_str());
    assert(info && "No irq info found");
    assert(!info->mapped_by.empty());
    return false;
}



void XApic::foundLocalAPIC(void* reg_phys_addr, MADTExtendedHeader::Flags flags)
{
    debug(APIC, "Local APIC at phys %p, PIC 8259: %u\n", reg_paddr_, flags.PCAT_COMPAT);
    PIC8259::exists = flags.PCAT_COMPAT;
    assert(reg_phys_addr == readMsrPhysAddr());
}

void XApic::mapAt(size_t addr)
{
    assert(addr);
    assert(cpu_features.cpuHasFeature(CpuFeatures::APIC));

    debug(APIC, "Map local APIC at phys %p to %p\n", reg_paddr_, (void*)addr);

    ArchMemoryMapping m = kernel_arch_mem.resolveMapping(addr/PAGE_SIZE);
    if (!m.page)
    {
        assert(ArchMemory::mapKernelPage(addr/PAGE_SIZE, ((size_t)reg_paddr_)/PAGE_SIZE, true, true));
    }
    else
    {
        debug(APIC, "Vaddr %p is already mapped to ppn %zx\n", (void*)addr, m.page_ppn);
        assert(m.page_ppn == ((size_t)reg_paddr_)/PAGE_SIZE);
        assert(m.pt[m.pti].write_through);
        assert(m.pt[m.pti].cache_disabled);
    }
    reg_vaddr_ = (void*)addr;
}

void XApic::init()
{
    debug(APIC, "Initializing local xAPIC\n");
    assert(!isInitialized());

    id_ = readId();
    auto logical_dest_id = readRegister<Register::LOGICAL_DESTINATION>();
    debug(APIC, "Local xAPIC, id: %x, logical dest: %x\n", apicId(), logical_dest_id);

    setErrorInterruptVector(ERROR_INTERRUPT_VECTOR);
    setSpuriousInterruptNumber(100);

    enable(true);

    initialized_ = true;
    SMP::currentCpu().setId(id_);

    registerIPI(90);
    registerIPI(91);
    registerIPI(100);
    registerIPI(101);
}

void Apic::LVT_TimerRegister::setVector(uint8 num)
{
    debug(APIC, "Set timer interrupt number %x\n", num);
    assert(num >= 32);
    vector = num;
}

void Apic::LVT_TimerRegister::setMode(TimerMode mode)
{
    debug(APIC, "Set timer mode %x\n", (uint32_t)mode);
    timer_mode = (uint32_t)mode;
}

void Apic::LVT_TimerRegister::setMask(bool new_mask)
{
    debug(APIC, "Set timer mask %u\n", new_mask);
    mask = new_mask;
}

void Apic::SpuriousInterruptVectorRegister::setSpuriousInterruptNumber(uint8 num)
{
    debug(APIC, "Set spurious interrupt number %x\n", num);
    vector = num;
}

void Apic::TimerDivideConfigRegister::setTimerDivisor(uint8 divisor)
{
    debug(APIC, "Set timer divisor %x\n", divisor);

    switch(divisor)
    {
    case 1:
        divisor_l = 0b11;
        divisor_h = 1;
        break;
    case 2:
        divisor_l = 0b00;
        divisor_h = 0;
        break;
    case 4:
        divisor_l = 0b01;
        divisor_h = 0;
        break;
    case 8:
        divisor_l = 0b10;
        divisor_h = 0;
        break;
    case 16:
        divisor_l = 0b11;
        divisor_h = 0;
        break;
    case 32:
        divisor_l = 0b00;
        divisor_h = 1;
        break;
    case 64:
        divisor_l = 0b01;
        divisor_h = 1;
        break;
    case 128:
        divisor_l = 0b10;
        divisor_h = 1;
        break;
    default:
        assert(false);
        break;
    }
}



uint32 XApic::readId()
{
    auto id = readRegister<Register::ID>();
    assert(id.xapic_id == CPUID::localApicId());
    return id.xapic_id;
}


static volatile uint8 delay = 0;

// TODO: Make this really architecture independent
#ifdef CMAKE_X86_64
#define IRET __asm__ __volatile__("iretq\n");
#else
#define IRET __asm__ __volatile__("iretl\n");
#endif

extern "C" void PIT_delay_IRQ();
__attribute__((naked)) void __PIT_delay_IRQ()
{
    __asm__ __volatile__(".global PIT_delay_IRQ\n"
                         ".type PIT_delay_IRQ,@function\n"
                         "PIT_delay_IRQ:\n");
    __asm__ __volatile__("movb $1, %[delay]\n"
                         :[delay]"=m"(delay));
    IRET
}

extern "C" void arch_irqHandler_0();

void Apic::startAP(uint8 apic_id, size_t entry_addr)
{
    debug(A_MULTICORE, "Sending init IPI to AP local APIC %u, AP entry function: %zx\n",  apic_id, entry_addr);

    assert((entry_addr % PAGE_SIZE) == 0);
    assert((entry_addr/PAGE_SIZE) <= 0xFF);

    Apic::sendIPI(0, IPIDestination::TARGET, apic_id, IPIType::INIT);

    // 10ms delay

    InterruptGateDesc temp_irq0_descriptor = InterruptUtils::idt.entries[0x20];
    bool temp_using_apic_timer = usingAPICTimer();
    bool apic_timer_mask = timer_interrupt_controller.isMasked();
    ArchInterrupts::disableIRQ(PIT::instance().irq());

    if (temp_using_apic_timer)
    {
        if (!apic_timer_mask)
        {
            SMP::currentCpu().rootIrqDomain().activateIrq(APIC_TIMER_VECTOR, false);
            assert(timer_interrupt_controller.isMasked());
        }
        setUsingAPICTimer(false);
    }

    auto old_pit_mode = PIT::setOperatingMode(PIT::OperatingMode::ONESHOT);
    auto old_pit_freq = PIT::frequencyDivisor();

    InterruptUtils::idt.entries[IRQ_VECTOR_OFFSET + 0].setOffset(&PIT_delay_IRQ);

    ArchInterrupts::enableIRQ(PIT::instance().irq());
    ArchInterrupts::startOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 0);
    ArchInterrupts::enableInterrupts();

    PIT::setOperatingMode(PIT::OperatingMode::ONESHOT);
    PIT::setFrequencyDivisor(1193182 / 100);
    debugAdvanced(A_MULTICORE, "Waiting for delay 1\n");
    assert(ArchInterrupts::testIFSet());
    while(!delay);

    ArchInterrupts::disableInterrupts();
    ArchInterrupts::disableIRQ(PIT::instance().irq());
    ArchInterrupts::endOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 0);

    delay = 0;

    sendIPI(entry_addr/PAGE_SIZE, IPIDestination::TARGET, apic_id, IPIType::SIPI);

    // 200us delay

    ArchInterrupts::enableIRQ(PIT::instance().irq());
    ArchInterrupts::startOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 0);
    ArchInterrupts::enableInterrupts();

    PIT::setFrequencyDivisor(1193182 / 100);
    debugAdvanced(A_MULTICORE, "Waiting for delay 2\n");
    while(!delay);

    ArchInterrupts::disableInterrupts();
    ArchInterrupts::disableIRQ(PIT::instance().irq());
    ArchInterrupts::endOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 0);

    delay = 0;

    // Second SIPI just in case the first one didn't work
    sendIPI(entry_addr/PAGE_SIZE, IPIDestination::TARGET, apic_id, IPIType::SIPI);

    if (temp_using_apic_timer)
    {
        setUsingAPICTimer(temp_using_apic_timer);
        SMP::currentCpu().rootIrqDomain().activateIrq(APIC_TIMER_VECTOR, !apic_timer_mask);
    }

    PIT::setOperatingMode(old_pit_mode);
    PIT::setFrequencyDivisor(old_pit_freq);

    InterruptUtils::idt.entries[IRQ_VECTOR_OFFSET + 0] = temp_irq0_descriptor;

    debugAdvanced(A_MULTICORE, "Finished sending IPI to AP local APIC\n");
}

void XApic::waitIpiDelivered()
{
    InterruptCommandRegister icr{};
    while(icr.l.delivery_status == (uint32_t)DeliveryStatus::PENDING)
    {
        icr = readRegister<Register::INTERRUPT_COMMAND>();
    }
}

void Apic::registerIPI(irqnum_t irqnum)
{
    inter_processor_interrupt_domain.irq(irqnum).mapTo(*this, irqnum);
}

void Apic::sendIPI(uint8_t vector, IPIDestination dest_type, size_t target, IPIType ipi_type, bool wait_for_delivery)
{
    assert(isInitialized());
    assert(!((ipi_type == IPIType::FIXED) && (vector < 32)));

    // Need to ensure this section of code runs on the same CPU and the APIC is not used for anything else in the meantime
    WithInterrupts d(false);

    if (A_MULTICORE & OUTPUT_ADVANCED)
        debug(APIC, "CPU %x Sending IPI, vector: %x\n", apicId(), vector);

    InterruptCommandRegisterLow icrl{};
    icrl.vector                = vector;
    icrl.delivery_mode         = (uint32_t)ipi_type;
    icrl.destination_mode      = (uint32_t)IPIDestinationMode::PHYSICAL;
    icrl.level                 = (uint32_t)IPILevel::ASSERT;
    icrl.trigger_mode          = (uint32_t)IntTriggerMode::EDGE;
    icrl.destination_shorthand = (uint32_t)dest_type;

    writeIcr(icrl, dest_type == IPIDestination::TARGET ? target : 0);

    if(wait_for_delivery && !((dest_type == IPIDestination::TARGET) && (target == apicId())))
    {
        debug(APIC, "CPU %zx waiting until IPI to %zx has been delivered\n", SMP::currentCpuId(), target);
        waitIpiDelivered();
    }
}

void Apic::sendIPI(uint8_t vector, const Apic& target, bool wait_for_delivery)
{
    assert(isInitialized());
    assert(target.isInitialized());
    sendIPI(vector, IPIDestination::TARGET, target.apicId(), IPIType::FIXED, wait_for_delivery);
}


void XApic::writeRegisterImpl(ApicRegisterOffset offset, uint64_t v)
{
    assert(offset != ApicRegisterOffset::SELF_IPI);

    WithInterrupts i{false};
    if (offset == ApicRegisterOffset::INTERRUPT_COMMAND)
    {
        volatile uint32_t* icr_h = (uint32_t*)((char*)reg_vaddr_ + static_cast<unsigned int>(ApicRegisterOffset::INTERRUPT_COMMAND_H));
        *icr_h = (v >> 32);
    }

    volatile uint32_t* reg_addr = (uint32_t*)((char*)reg_vaddr_ + static_cast<unsigned int>(offset));
    uint32_t v32 = v;
    *reg_addr = v32;
}

uint64_t XApic::readRegisterImpl(ApicRegisterOffset offset)
{
    assert(offset != ApicRegisterOffset::SELF_IPI);

    uint64_t v = 0;

    WithInterrupts i{false};

    if (offset == ApicRegisterOffset::INTERRUPT_COMMAND)
    {
        uint32_t* icr_h = (uint32_t*)((char*)reg_vaddr_ + static_cast<unsigned int>(ApicRegisterOffset::INTERRUPT_COMMAND_H));

        v = (uint64_t)*icr_h << 32;
    }

    volatile uint32_t* reg_addr = (uint32_t*)((char*)reg_vaddr_ + static_cast<unsigned int>(offset));
    v |= *reg_addr;
    return v;
}

void XApic::writeIcr(InterruptCommandRegisterLow icr_l, uint32_t dest)
{
    InterruptCommandRegister icr{};
    icr.h.xapic_destination = dest;
    icr.l = icr_l;

    WithInterrupts d(false);
    writeRegister<Register::INTERRUPT_COMMAND>(icr);
}

void* XApic::readMsrPhysAddr()
{
    auto apic_base = MSR::IA32_APIC_BASE::read();
    return (void*)((uintptr_t)apic_base.apic_base*PAGE_SIZE);
}

bool XApic::apicSupported()
{
    return cpu_features.cpuHasFeature(CpuFeatures::X86Feature::APIC);
}

Apic::ApicTimer::ApicTimer(Apic& apic) :
    IrqDomain("APIC Timer", 1, this),
    Device("APIC timer", &apic),
    apic_(&apic)
{
}

bool Apic::ApicTimer::mask(irqnum_t irq, bool mask)
{
    assert(irq == 0);
    WithInterrupts i{false};
    auto timer_reg = apic_->readRegister<Register::LVT_TIMER>();

    debug(APIC, "[Cpu %u] %s mask IRQ %zx = %u -> %u\n", apic_->apicId(), name().c_str(), irq, timer_reg.mask, mask);

    timer_reg.mask = mask;
    masked_ = mask;
    apic_->writeRegister<Register::LVT_TIMER>(timer_reg);
    return true;
}

bool Apic::ApicTimer::ack(irqnum_t irq)
{
    debugAdvanced(APIC, "[Cpu %u] %s ack IRQ %zx\n", apic_->apicId(), name().c_str(), irq);
    assert(irq == 0);
    --pending_EOIs;
    apic_->sendEOI(Apic::APIC_TIMER_VECTOR);
    return true;
}

bool Apic::ApicTimer::isMasked(irqnum_t irq)
{
    assert(irq == 0);
    return isMasked();
}

bool Apic::ApicTimer::isMasked() { return masked_; }

ApicDriver::ApicDriver() :
    BasicDeviceDriver("Local APIC driver")
{
}

ApicDriver &ApicDriver::instance()
{
  static ApicDriver i;
  return i;
}

void ApicDriver::doDeviceDetection()
{
    debug(APIC, "Device detection\n");
    cpuLocalInit();
}

void ApicDriver::cpuLocalInit()
{
    if (cpu_features.cpuHasFeature(CpuFeatures::APIC))
    {
        debug(APIC, "Init cpu local apic\n");
        Apic::setIMCRMode(IMCRData::APIC_PASSTHROUGH);

        Apic::globalEnable();
        if (X2Apic::x2ApicSupported())
        {
            X2Apic::enableX2ApicMode();
        }
        else
        {
            XApic::setPhysicalAddress(XApic::readMsrPhysAddr());
            if (!XApic::virtualAddress())
            {
                auto apic_vaddr = mmio_addr_allocator.alloc(PAGE_SIZE, PAGE_SIZE);
                debug(APIC, "Allocated MMIO addr for APIC: %zx\n", apic_vaddr);
                assert(apic_vaddr != (size_t)-1 && "Unable to allocate virtual page for APIC MMIO");
                XApic::mapAt(apic_vaddr);
            }
        }

        cpu_root_irq_domain_ = cpu_lapic;
        cpu_lapic->init();

        SMP::currentCpu().addSubDevice(*cpu_lapic);
        bindDevice(*cpu_lapic);
    }
    else
    {
        debug(APIC, "Local APIC not available\n");
    }
}

ApicTimerDriver::ApicTimerDriver() :
    BasicDeviceDriver("APIC timer driver")
{
}

ApicTimerDriver& ApicTimerDriver::instance()
{
    static ApicTimerDriver i;
    return i;
}

void ApicTimerDriver::doDeviceDetection()
{
	cpuLocalInit();
}

void ApicTimerDriver::cpuLocalInit()
{
    if (cpu_features.cpuHasFeature(CpuFeatures::APIC) && cpu_lapic)
    {
        auto& timer = cpu_lapic->timer_interrupt_controller;
        timer.setDeviceName(eastl::string("APIC ") + eastl::to_string(cpu_lapic->apicId()) + " timer");
        cpu_lapic->initTimer();

        timer.irq().mapTo(*cpu_lapic, Apic::APIC_TIMER_VECTOR);

        bindDevice(timer);
    }
    else
    {
        debug(APIC, "Local APIC timer not available\n");
    }
}
