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


void* XApic::reg_paddr_ = (void*)0xfee00000;
void* XApic::reg_vaddr_ = (void*)0xfee00000;

eastl::vector<MADTProcLocalAPIC> Apic::local_apic_list_{};

extern volatile size_t outstanding_EOIs;

uint32 Apic::Id() const
{
    return id_;
}

bool Apic::isInitialized() const
{
    return initialized_;
}

void Apic::globalEnable(bool enable)
{
    auto apic_base = MSR::IA32_APIC_BASE::read();
    apic_base.enable = enable;
    MSR::IA32_APIC_BASE::write(apic_base);
}

void Apic::enable(bool enable)
{
    debug(APIC, "%s APIC %x\n", (enable ? "Enabling" : "Disabling"), Id());

    WithInterrupts i(false);
    auto siv = readRegister<Register::SPURIOUS_INTERRUPT_VECTOR>();
    siv.enable = enable;
    writeRegister<Register::SPURIOUS_INTERRUPT_VECTOR>(siv);
}

void Apic::initTimer()
{
    debug(APIC, "Init timer for APIC %x\n", Id());
    assert(!ArchInterrupts::testIFSet());

    setUsingAPICTimer(true);

    auto div = readRegister<Register::TIMER_DIVIDE_CONFIG>();
    div.setTimerDivisor(TIMER_DIVISOR);
    writeRegister<Register::TIMER_DIVIDE_CONFIG>(div);

    auto timer_reg = readRegister<Register::LVT_TIMER>();
    timer_reg.setVector(0x20);
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

void Apic::setUsingAPICTimer(bool using_apic_timer)
{
    debug(APIC, "Using APIC timer: %d\n", using_apic_timer);
    use_apic_timer_ = using_apic_timer;
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

    assert(ArchMemory::mapKernelPage(addr/PAGE_SIZE, ((size_t)reg_paddr_)/PAGE_SIZE, true, true));
    reg_vaddr_ = (void*)addr;
}

void XApic::init()
{
    debug(APIC, "Initializing local xAPIC\n");
    assert(!isInitialized());

    id_ = readId();
    auto logical_dest_id = readRegister<Register::LOGICAL_DESTINATION>();
    debug(APIC, "Local xAPIC, id: %x, logical dest: %x\n", Id(), logical_dest_id);

    setErrorInterruptVector(ERROR_INTERRUPT_VECTOR);
    setSpuriousInterruptNumber(100);
    initTimer();
    enable(true);

    initialized_ = true;
    current_cpu.setId(id_);
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

    PIT::PITCommandRegister pit_command{};
    pit_command.bcd_mode = 0;
    pit_command.operating_mode = 0; // oneshot
    pit_command.access_mode = 3; // send low + high byte of reload value/divisor
    pit_command.channel = 0;

    InterruptGateDesc temp_irq0_descriptor = InterruptUtils::idt[0x20];
    bool temp_using_apic_timer = usingAPICTimer();
    setUsingAPICTimer(false);

    InterruptUtils::idt[0x20].setOffset((size_t)&PIT_delay_IRQ);

    ArchInterrupts::enableIRQ(0);
    ArchInterrupts::startOfInterrupt(0);
    ArchInterrupts::enableInterrupts();

    PIT::init(pit_command.value, 1193182 / 100);
    while(!delay);

    ArchInterrupts::disableInterrupts();
    ArchInterrupts::disableIRQ(0);
    ArchInterrupts::endOfInterrupt(0);

    delay = 0;

    sendIPI(entry_addr/PAGE_SIZE, IPIDestination::TARGET, apic_id, IPIType::SIPI);

    // 200us delay

    ArchInterrupts::enableIRQ(0);
    ArchInterrupts::startOfInterrupt(0);
    ArchInterrupts::enableInterrupts();

    PIT::init(pit_command.value, 1193182 / 5000);
    while(!delay);

    ArchInterrupts::disableInterrupts();
    ArchInterrupts::disableIRQ(0);
    ArchInterrupts::endOfInterrupt(0);

    delay = 0;

    // Second SIPI just in case the first one didn't work
    sendIPI(entry_addr/PAGE_SIZE, IPIDestination::TARGET, apic_id, IPIType::SIPI);

    setUsingAPICTimer(temp_using_apic_timer);
    InterruptUtils::idt[0x20] = temp_irq0_descriptor;

    if (A_MULTICORE & OUTPUT_ADVANCED)
        debug(A_MULTICORE, "Finished sending IPI to AP local APIC\n");
}

void XApic::waitIpiDelivered()
{
    InterruptCommandRegister icr{};
    while(icr.l.delivery_status == (uint32_t)DeliveryStatus::PENDING)
    {
        icr = readRegister<Register::INTERRUPT_COMMAND>();
    }
}

void Apic::sendIPI(uint8_t vector, IPIDestination dest_type, size_t target, IPIType ipi_type, bool wait_for_delivery)
{
    assert(isInitialized());
    assert(!((ipi_type == IPIType::FIXED) && (vector < 32)));

    // Need to ensure this section of code runs on the same CPU and the APIC is not used for anything else in the meantime
    WithInterrupts d(false);

    if (A_MULTICORE & OUTPUT_ADVANCED)
        debug(APIC, "CPU %x Sending IPI, vector: %x\n", Id(), vector);

    InterruptCommandRegisterLow icrl{};
    icrl.vector                = vector;
    icrl.delivery_mode         = (uint32_t)ipi_type;
    icrl.destination_mode      = (uint32_t)IPIDestinationMode::PHYSICAL;
    icrl.level                 = (uint32_t)IPILevel::ASSERT;
    icrl.trigger_mode          = (uint32_t)IntTriggerMode::EDGE;
    icrl.destination_shorthand = (uint32_t)dest_type;

    writeIcr(icrl, dest_type == IPIDestination::TARGET ? target : 0);

    if(wait_for_delivery && !((dest_type == IPIDestination::TARGET) && (target == Id())))
    {
        debug(APIC, "CPU %zx waiting until IPI to %zx has been delivered\n", SMP::currentCpuId(), target);
        waitIpiDelivered();
    }
}

void Apic::sendIPI(uint8_t vector, const Apic& target, bool wait_for_delivery)
{
    assert(isInitialized());
    assert(target.isInitialized());
    sendIPI(vector, IPIDestination::TARGET, target.Id(), IPIType::FIXED, wait_for_delivery);

    // // Ensure this section of code runs on the same CPU and the local APIC is not used for anything else in the meantime
    // WithInterrupts d(false);

    // if (A_MULTICORE & OUTPUT_ADVANCED)
    //     debug(APIC, "CPU %x sending IPI to CPU %x, vector: %x\n", Id(), target.Id(), vector);

    // InterruptCommandRegisterLow icrl{};
    // icrl.vector                = vector;
    // icrl.delivery_mode         = (uint32)IPIType::FIXED;
    // icrl.destination_mode      = (uint32)IPIDestinationMode::PHYSICAL;
    // icrl.level                 = (uint32)IPILevel::ASSERT;
    // icrl.trigger_mode          = (uint32)IntTriggerMode::EDGE;
    // icrl.destination_shorthand = (uint32)IPIDestination::TARGET;
    // icrl.delivery_status       = (uint32)DeliveryStatus::IDLE;


    // writeIcr(icrl, target.Id());

    // if(wait_for_delivery && (target.Id() != Id()))
    // {
    //     waitIpiDelivered();
    // }
}


void XApic::writeRegisterImpl(ApicRegisterOffset offset, uint64_t v)
{
    assert(offset != ApicRegisterOffset::SELF_IPI);

    if (offset != ApicRegisterOffset::EOI)
        debug(APIC, "Write register %x, %lx\n", (unsigned int)offset, v);

    WithInterrupts i{false};
    if (offset == ApicRegisterOffset::INTERRUPT_COMMAND)
    {
        volatile uint32_t* icr_h = (uint32_t*)((char*)reg_vaddr_ + static_cast<unsigned int>(ApicRegisterOffset::INTERRUPT_COMMAND_H));
        *icr_h = (v >> 32);
    }

    volatile uint32_t* reg_addr = (uint32_t*)((char*)reg_vaddr_ + static_cast<unsigned int>(offset));
    uint32_t v32 = v;
    *reg_addr = v32;
    if (offset != ApicRegisterOffset::EOI)
        debug(APIC, "Register write complete\n");
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
