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
#include "offsets.h"


bool LocalAPIC::exists = false;
LocalAPICRegisters* LocalAPIC::reg_paddr_ = nullptr;
LocalAPICRegisters* LocalAPIC::reg_vaddr_ = nullptr;
eastl::vector<MADTProcLocalAPIC> LocalAPIC::local_apic_list_{};

extern volatile size_t outstanding_EOIs;

LocalAPIC::LocalAPIC() :
        outstanding_EOIs_(0)
{
}

void LocalAPIC::haveLocalAPIC(LocalAPICRegisters* reg_phys_addr, uint32 flags)
{
  reg_paddr_ = reg_phys_addr;
  reg_vaddr_ = reg_phys_addr;
  exists = true;
  debug(APIC, "Local APIC at phys %p, flags: %x\n", reg_paddr_, flags);
}

void LocalAPIC::sendEOI(size_t num)
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

  reg_vaddr_->eoi = 0;
}


void LocalAPIC::mapAt(size_t addr)
{
  assert(addr);
  assert(exists);

  debug(APIC, "Map local APIC at phys %p to %p\n", reg_paddr_, (void*)addr);

  assert(ArchMemory::mapKernelPage(addr/PAGE_SIZE, ((size_t)reg_paddr_)/PAGE_SIZE, true, true));
  reg_vaddr_ = (LocalAPICRegisters*)addr;
}


void LocalAPIC::init()
{
  debug(APIC, "Initializing Local APIC\n");
  if(!isInitialized())
  {
    id_ = readID();
    debug(APIC, "Local APIC %x\n", ID());
    setSpuriousInterruptNumber(100);
    initTimer();
    enable(true);
    initialized_ = true;
    current_cpu.setId(id_);
  }
  else
  {
    debug(APIC, "Local APIC %x is already initialized. Skipping re-initialization\n", ID());
  }
}

bool LocalAPIC::isInitialized() const volatile
{
  return initialized_;
}

void LocalAPIC::enable(bool enable)
{
  debug(APIC, "%s APIC %x\n", (enable ? "Enabling" : "Disabling"), ID());

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
  uint32* ptr = (uint32*)&reg_vaddr_->s_int_vect;
  uint32 temp = *ptr;

  ((LocalAPIC_SpuriousInterruptVector*)&temp)->enable = (enable ? 1 : 0);
  *ptr = temp;
#pragma GCC diagnostic pop

}

void LocalAPIC::initTimer() volatile
{
        debug(APIC, "Init timer for APIC %x\n", ID());
        assert(!ArchInterrupts::testIFSet());
        reg_vaddr_->lvt_timer.setVector(0x20);
        reg_vaddr_->lvt_timer.setMode(1);
        reg_vaddr_->lvt_timer.setMask(true);
        reg_vaddr_->timer_divide_config.setTimerDivisor(16);
        setTimerPeriod(0x500000);
        use_apic_timer_ = true;
}

void LocalAPIC::setUsingAPICTimer(bool using_apic_timer)  volatile
{
        debug(APIC, "Using APIC timer: %d\n", using_apic_timer);
        use_apic_timer_ = using_apic_timer;
}

bool LocalAPIC::usingAPICTimer() const volatile
{
        return use_apic_timer_;
}


void LocalAPIC_LVT_TimerRegister::setVector(uint8 num) volatile
{
        debug(APIC, "Set timer interrupt number %x\n", num);
        assert(num > 16);
        uint32 temp = *(uint32*)this;
        ((LocalAPIC_LVT_TimerRegister*)&temp)->vector = num;
        *(uint32*)this = temp;
}

void LocalAPIC_LVT_TimerRegister::setMode(uint8 mode) volatile
{
        debug(APIC, "Set timer mode %x\n", mode);
        assert((mode == 0) || (mode == 1) || (mode == 3));
        uint32 temp = *(uint32*)this;
        ((LocalAPIC_LVT_TimerRegister*)&temp)->timer_mode = mode;
        *(uint32*)this = temp;
}

void LocalAPIC_LVT_TimerRegister::setMask(bool new_mask) volatile
{
        debug(APIC, "Set timer mask %u\n", new_mask);
        uint32 temp = *(uint32*)this;
        ((LocalAPIC_LVT_TimerRegister*)&temp)->mask = (new_mask ? 1 : 0);
        *(uint32*)this = temp;
}

void LocalAPIC::setTimerPeriod(uint32 count) volatile
{
        debug(APIC, "Set timer period %x\n", count);
        reg_vaddr_->init_timer_count = count;
}

void LocalAPIC::setSpuriousInterruptNumber(uint8 num) volatile
{
        reg_vaddr_->s_int_vect.setSpuriousInterruptNumber(num);
}

void LocalAPIC_SpuriousInterruptVector::setSpuriousInterruptNumber(uint8 num) volatile
{
        debug(APIC, "Set spurious interrupt number %x\n", num);
        uint32 temp = *(uint32*)this;
        ((LocalAPIC_SpuriousInterruptVector*)&temp)->vector = num;
        *(uint32*)this = temp;
}


void LocalAPIC_TimerDivideConfigRegister::setTimerDivisor(uint8 divisor) volatile
{
        debug(APIC, "Set timer divisor %x\n", divisor);
        uint32 temp = *(uint32*)this;

        switch(divisor)
        {
        case 1:
                ((LocalAPIC_TimerDivideConfigRegister*)&temp)->divisor_l = 0b11;
                ((LocalAPIC_TimerDivideConfigRegister*)&temp)->divisor_h = 1;
                break;
        case 2:
                ((LocalAPIC_TimerDivideConfigRegister*)&temp)->divisor_l = 0b00;
                ((LocalAPIC_TimerDivideConfigRegister*)&temp)->divisor_h = 0;
                break;
        case 4:
                ((LocalAPIC_TimerDivideConfigRegister*)&temp)->divisor_l = 0b01;
                ((LocalAPIC_TimerDivideConfigRegister*)&temp)->divisor_h = 0;
                break;
        case 8:
                ((LocalAPIC_TimerDivideConfigRegister*)&temp)->divisor_l = 0b10;
                ((LocalAPIC_TimerDivideConfigRegister*)&temp)->divisor_h = 0;
                break;
        case 16:
                ((LocalAPIC_TimerDivideConfigRegister*)&temp)->divisor_l = 0b11;
                ((LocalAPIC_TimerDivideConfigRegister*)&temp)->divisor_h = 0;
                break;
        case 32:
                ((LocalAPIC_TimerDivideConfigRegister*)&temp)->divisor_l = 0b00;
                ((LocalAPIC_TimerDivideConfigRegister*)&temp)->divisor_h = 1;
                break;
        case 64:
                ((LocalAPIC_TimerDivideConfigRegister*)&temp)->divisor_l = 0b01;
                ((LocalAPIC_TimerDivideConfigRegister*)&temp)->divisor_h = 1;
                break;
        case 128:
                ((LocalAPIC_TimerDivideConfigRegister*)&temp)->divisor_l = 0b10;
                ((LocalAPIC_TimerDivideConfigRegister*)&temp)->divisor_h = 1;
                break;
        default:
                assert(false);
                break;
        }
        *(uint32*)this = temp;
}


bool LocalAPIC::checkIRR(uint8 num) volatile
{
        uint8 byte_offset = num/32;
        uint8 bit_offset = num % 32;

        return reg_vaddr_->IRR[byte_offset].irr & (1 << bit_offset);
}

bool LocalAPIC::checkISR(uint8 num) volatile
{
        uint8 byte_offset = num/32;
        uint8 bit_offset = num % 32;

        return reg_vaddr_->ISR[byte_offset].isr & (1 << bit_offset);
}

uint32 LocalAPIC::readID() volatile
{
        LocalAPIC_IDRegister id;
        *(uint32*)&id = *(uint32*)&reg_vaddr_->local_apic_id;
        return id.id;
}

uint32 LocalAPIC::ID() const volatile
{
        return id_;
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

void LocalAPIC::startAP(uint8 apic_id, size_t entry_addr) volatile
{
        debug(A_MULTICORE, "Sending init IPI to AP local APIC %u, AP entry function: %zx\n",  apic_id, entry_addr);

        sendIPI(0, LAPIC::IPIDestination::TARGET, apic_id, LAPIC::IPIType::INIT);

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

        assert((entry_addr % PAGE_SIZE) == 0);
        assert((entry_addr/PAGE_SIZE) <= 0xFF);

        sendIPI(entry_addr/PAGE_SIZE, LAPIC::IPIDestination::TARGET, apic_id, LAPIC::IPIType::SIPI);

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
        sendIPI(entry_addr/PAGE_SIZE, LAPIC::IPIDestination::TARGET, apic_id, LAPIC::IPIType::SIPI);

        setUsingAPICTimer(temp_using_apic_timer);
        InterruptUtils::idt[0x20] = temp_irq0_descriptor;

        if (A_MULTICORE & OUTPUT_ADVANCED)
            debug(A_MULTICORE, "Finished sending IPI to AP local APICs\n");
}


void LocalAPIC::sendIPI(uint8 vector, LAPIC::IPIDestination dest_type, size_t target, LAPIC::IPIType ipi_type, bool wait_for_delivery) volatile
{
        assert(isInitialized());
        assert(!((ipi_type == LAPIC::IPIType::FIXED) && (vector < 32)));

        // Need to ensure this section of code runs on the same CPU and the APIC is not used for anything else in the meantime
        WithInterrupts d(false);

        if (A_MULTICORE & OUTPUT_ADVANCED)
            debug(APIC, "CPU %x Sending IPI, vector: %x\n", ID(), vector);

        LocalAPIC_InterruptCommandRegisterHigh v_high{};
        v_high.destination = (dest_type == LAPIC::IPIDestination::TARGET ? target : 0);

        LocalAPIC_InterruptCommandRegisterLow v_low{};
        v_low.vector                = vector;
        v_low.delivery_mode         = (uint32)ipi_type;
        v_low.destination_mode      = (uint32)LAPIC::IPIDestinationMode::PHYSICAL;
        v_low.level                 = (uint32)LAPIC::IPILevel::ASSERT;
        v_low.trigger_mode          = (uint32)LAPIC::IntTriggerMode::EDGE;
        v_low.destination_shorthand = (uint32)dest_type;


        *(volatile uint32*)&reg_vaddr_->ICR_high  = *(uint32*)&v_high;
        *(volatile uint32*)&reg_vaddr_->ICR_low  = *(uint32*)&v_low;

        if(wait_for_delivery && !((dest_type == LAPIC::IPIDestination::TARGET) && (target == ID())))
        {
                debug(APIC, "CPU %zx waiting until IPI to %zx has been delivered\n", SMP::currentCpuId(), target);
                while(v_low.delivery_status == (uint32)LAPIC::DeliveryStatus::PENDING)
                {
                        *(uint32*)&v_low = *(volatile uint32*)&reg_vaddr_->ICR_low;
                }
        }
}

void LocalAPIC::sendIPI(uint8 vector, const LocalAPIC& target, bool wait_for_delivery) volatile
{
        assert(isInitialized());
        assert(target.isInitialized());
        assert(!(vector < 32));

        // Ensure this section of code runs on the same CPU and the local APIC is not used for anything else in the meantime
        WithInterrupts d(false);

        if (A_MULTICORE & OUTPUT_ADVANCED)
            debug(APIC, "CPU %x sending IPI to CPU %x, vector: %x\n", ID(), target.ID(), vector);

        LocalAPIC_InterruptCommandRegisterHigh v_high{};
        v_high.destination = target.ID();

        LocalAPIC_InterruptCommandRegisterLow v_low{};
        v_low.vector                = vector;
        v_low.delivery_mode         = (uint32)LAPIC::IPIType::FIXED;
        v_low.destination_mode      = (uint32)LAPIC::IPIDestinationMode::PHYSICAL;
        v_low.level                 = (uint32)LAPIC::IPILevel::ASSERT;
        v_low.trigger_mode          = (uint32)LAPIC::IntTriggerMode::EDGE;
        v_low.destination_shorthand = (uint32)LAPIC::IPIDestination::TARGET;
        v_low.delivery_status       = (uint32)LAPIC::DeliveryStatus::IDLE;


        *(volatile uint32*)&reg_vaddr_->ICR_high  = *(uint32*)&v_high;
        *(volatile uint32*)&reg_vaddr_->ICR_low  = *(uint32*)&v_low;

        if(wait_for_delivery && (target.ID() != ID()))
        {
                while(v_low.delivery_status == (uint32)LAPIC::DeliveryStatus::PENDING)
                {
                        debug(APIC, "CPU %zx waiting until IPI to %x has been delivered\n", SMP::currentCpuId(), target.ID());
                        *(uint32*)&v_low = *(volatile uint32*)&reg_vaddr_->ICR_low;
                }
        }
}
