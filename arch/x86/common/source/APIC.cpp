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


bool LocalAPIC::exists = false;
LocalAPICRegisters* LocalAPIC::reg_paddr_ = nullptr;
LocalAPICRegisters* LocalAPIC::reg_vaddr_ = nullptr;
ustl::vector<MADTProcLocalAPIC> LocalAPIC::local_apic_list_{};

ustl::vector<MADTInterruptSourceOverride> IOAPIC::irq_source_override_list_{};
ustl::vector<IOAPIC> IOAPIC::io_apic_list_{};

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

void LocalAPIC::sendEOI(__attribute__((unused)) size_t num)
{
  --outstanding_EOIs_;
  if(APIC & OUTPUT_ADVANCED)
  {
    debug(APIC, "CPU %zu, Sending EOI for %zx\n", ArchMulticore::getCpuID(), num);
    for(size_t i = 0; i < 256; ++i)
    {
            if(checkISR(i))
            {
                    debug(APIC, "CPU %zx, interrupt %zx being serviced\n", ArchMulticore::getCpuID(), i);
            }
    }
    for(size_t i = 0; i < 256; ++i)
    {
            if(checkIRR(i))
            {
                    debug(APIC, "CPU %zx, interrupt %zx pending\n", ArchMulticore::getCpuID(), i);
            }
    }
  }

  assert(!ArchInterrupts::testIFSet());
  assert(checkISR(num));

  reg_vaddr_->eoi = 0;
}


void LocalAPIC::mapAt(size_t addr)
{
  assert(addr);
  assert(exists);

  debug(APIC, "Map local APIC at phys %p to %p\n", reg_paddr_, (void*)addr);

  ArchMemory::mapKernelPage(addr/PAGE_SIZE, ((size_t)reg_paddr_)/PAGE_SIZE, true, true);
  reg_vaddr_ = (LocalAPICRegisters*)addr;
}


void LocalAPIC::init()
{
  if(!isInitialized())
  {
    id_ = readID();
    debug(APIC, "Initializing Local APIC %x\n", ID());
    setSpuriousInterruptNumber(100);
    initTimer();
    enable(true);
    initialized_ = true;
    ArchMulticore::setCpuID(id_);
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
  uint32* ptr = (uint32*)&reg_vaddr_->s_int_vect;
  uint32 temp = *ptr;
  ((LocalAPIC_SpuriousInterruptVector*)&temp)->enable = (enable ? 1 : 0);
  *ptr = temp;
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

bool LocalAPIC::usingAPICTimer() volatile
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
void __PIT_delay_IRQ()
{
        __asm__ __volatile__(".global PIT_delay_IRQ\n"
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

        debug(A_MULTICORE, "Start delay 1\n");
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

        debug(A_MULTICORE, "End delay 1\n");

        assert((entry_addr % PAGE_SIZE) == 0);
        assert((entry_addr/PAGE_SIZE) <= 0xFF);

        sendIPI(entry_addr/PAGE_SIZE, LAPIC::IPIDestination::TARGET, apic_id, LAPIC::IPIType::SIPI);

        // 200us delay
        debug(A_MULTICORE, "Start delay 2\n");

        ArchInterrupts::enableIRQ(0);
        ArchInterrupts::startOfInterrupt(0);
        ArchInterrupts::enableInterrupts();

        PIT::init(pit_command.value, 1193182 / 5000);
        while(!delay);

        ArchInterrupts::disableInterrupts();
        ArchInterrupts::disableIRQ(0);
        ArchInterrupts::endOfInterrupt(0);

        delay = 0;

        debug(A_MULTICORE, "End delay 2\n");

        sendIPI(entry_addr/PAGE_SIZE, LAPIC::IPIDestination::TARGET, apic_id, LAPIC::IPIType::SIPI);

        // Wait another 10ms to give APs time for initialization
        ArchInterrupts::enableIRQ(0);
        ArchInterrupts::startOfInterrupt(0);
        ArchInterrupts::enableInterrupts();

        PIT::init(pit_command.value, 1193182 / 100);
        while(!delay);

        ArchInterrupts::disableInterrupts();
        ArchInterrupts::disableIRQ(0);
        ArchInterrupts::endOfInterrupt(0);


        setUsingAPICTimer(temp_using_apic_timer);
        InterruptUtils::idt[0x20] = temp_irq0_descriptor;

        debug(A_MULTICORE, "Finished sending IPI to AP local APICs\n");
}


void LocalAPIC::sendIPI(uint8 vector, LAPIC::IPIDestination dest_type, size_t target, LAPIC::IPIType ipi_type, bool wait_for_delivery) volatile
{
        assert(isInitialized());
        assert(!((ipi_type == LAPIC::IPIType::FIXED) && (vector < 32)));

        // Need to ensure this section of code runs on the same CPU and the APIC is not used for anything else in the meantime
        WithDisabledInterrupts d;

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
                debug(APIC, "CPU %zx waiting until IPI to %zx has been delivered\n", ArchMulticore::getCpuID(), target);
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
        WithDisabledInterrupts d;

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
                        debug(APIC, "CPU %zx waiting until IPI to %x has been delivered\n", ArchMulticore::getCpuID(), target.ID());
                        *(uint32*)&v_low = *(volatile uint32*)&reg_vaddr_->ICR_low;
                }
        }
}


IOAPIC::IOAPIC(uint32 id, IOAPIC_MMIORegs* regs, uint32 g_sys_int_base) :
        reg_paddr_(regs),
        reg_vaddr_(regs),
        id_(id),
        g_sys_int_base_(g_sys_int_base)
{
        debug(APIC, "IOAPIC %x at phys %p, g_sys_int_base: %x\n", id_, reg_paddr_, g_sys_int_base_);
        assert(reg_paddr_);
}

void IOAPIC::initAll()
{
        for(auto& io_apic : io_apic_list_)
        {
                io_apic.mapAt(IOAPIC_VADDR + io_apic.id_*PAGE_SIZE);
                io_apic.init();
        }
}

void IOAPIC::init()
{
        debug(A_INTERRUPTS, "Initializing I/O APIC\n");
        IOAPIC_r_ID id;
        IOAPIC_r_VER version;
        id.word = read(IOAPICID);
        version.word = read(IOAPICVER);
        max_redir_ = version.max_redir;
        debug(APIC, "IOAPIC id: %u, version: %#x, g_sys_ints: [%u, %u)\n", id.io_apic_id, version.version, getGlobalInterruptBase(), getGlobalInterruptBase() + getMaxRedirEntry());

        initRedirections();
}

void IOAPIC::initRedirections()
{
        for(uint32 i = 0; i <= max_redir_; ++i)
        {
                IOAPIC_redir_entry r = readRedirEntry(i);

                for(auto& entry : irq_source_override_list_)
                {
                        if(getGlobalInterruptBase() + i == entry.g_sys_int)
                        {
                                debug(APIC, "Found override for global system interrupt %2u -> IRQ SRC %u, trigger mode: %u, polarity: %u\n", entry.g_sys_int, entry.irq_source, entry.flags.trigger_mode, entry.flags.polarity);
                                r.interrupt_vector = IRQ_OFFSET + entry.irq_source;
                                r.polarity = (entry.flags.polarity == ACPI_MADT_POLARITY_ACTIVE_HIGH);
                                r.trigger_mode = (entry.flags.trigger_mode == ACPI_MADT_TRIGGER_LEVEL);
                                r.destination = cpu_info.lapic.ID();
                                goto write_entry;
                        }
                }

                r.interrupt_vector = getGlobalInterruptBase() + IRQ_OFFSET + i;

        write_entry:
                writeRedirEntry(i, r);
        }

        for(uint32 i = 0; i <= max_redir_; ++i)
        {
                IOAPIC_redir_entry r = readRedirEntry(i);
                debug(APIC, "IOAPIC redir entry: IRQ %2u -> vector %u, dest: %u, mask: %u, pol: %u, trig: %u\n", getGlobalInterruptBase() + i, r.interrupt_vector, r.destination, r.mask, r.polarity, r.trigger_mode);
        }
        debug(APIC, "IO APIC redirections initialized\n");
}


void IOAPIC::mapAt(size_t addr)
{
  debug(APIC, "Map IOAPIC %u at phys %p to %p\n", id_, reg_paddr_, (void*)addr);
  assert(addr);

  ArchMemory::mapKernelPage(addr/PAGE_SIZE, ((size_t)reg_paddr_)/PAGE_SIZE, true, true);
  reg_vaddr_ = (IOAPIC_MMIORegs*)addr;
}

uint32 IOAPIC::read(uint8 offset)
{
        WithDisabledInterrupts i;
        uint32 retval = 0;
        asm volatile("movl %[offset], %[io_reg_sel]\n"
                     "movl %[io_win], %[retval]\n"
                     :[io_reg_sel]"=m"(reg_vaddr_->io_reg_sel),
                      [retval]"=r"(retval)
                     :[offset]"r"((uint32)offset),
                      [io_win]"m"(reg_vaddr_->io_win));
        return retval;
}

void IOAPIC::write(uint8 offset, uint32 value)
{
        WithDisabledInterrupts i;
        asm volatile("movl %[offset], %[io_reg_sel]\n"
                     "movl %[value], %[io_win]\n"
                     :[io_reg_sel]"=m"(reg_vaddr_->io_reg_sel),
                      [io_win]"=m"(reg_vaddr_->io_win)
                     :[offset]"r"((uint32)offset),
                      [value]"r"(value));
}

uint8 IOAPIC::redirEntryOffset(uint32 entry_no)
{
        return (0x10 + 2 * entry_no);
}

IOAPIC::IOAPIC_redir_entry IOAPIC::readRedirEntry(uint32 entry_no)
{
        assert(entry_no <= max_redir_);
        uint8 offset = redirEntryOffset(entry_no);

        IOAPIC_redir_entry temp;
        temp.word_l = read(offset);
        temp.word_h = read(offset + 1);
        return temp;
}

void IOAPIC::writeRedirEntry(uint32 entry_no, const IOAPIC::IOAPIC_redir_entry& value)
{
        assert(entry_no <= max_redir_);
        uint8 offset = redirEntryOffset(entry_no);

        write(offset + 1, value.word_h);
        write(offset, value.word_l);
}

uint32 IOAPIC::getGlobalInterruptBase()
{
        return g_sys_int_base_;
}

uint32 IOAPIC::getMaxRedirEntry()
{
        return max_redir_;
}


void IOAPIC::setGSysIntMask(uint32 g_sys_int, bool value)
{
        debug(APIC, "Set G Sys Int %x mask: %u\n", g_sys_int, value);
        IOAPIC* io_apic = findIOAPICforGlobalInterrupt(g_sys_int);
        uint32 entry_offset = g_sys_int - io_apic->getGlobalInterruptBase();
        IOAPIC_redir_entry r = io_apic->readRedirEntry(entry_offset);
        r.mask = (value ? 1: 0);
        io_apic->writeRedirEntry(entry_offset, r);
}

void IOAPIC::setIRQMask(uint32 irq_num, bool value)
{
        debug(APIC, "Set IRQ %x mask: %u\n", irq_num, value);
        setGSysIntMask(findGSysIntForIRQ(irq_num), value);
}

IOAPIC* IOAPIC::findIOAPICforGlobalInterrupt(uint32 g_int)
{
        for(auto& io_apic : io_apic_list_)
        {
                uint32 base = io_apic.getGlobalInterruptBase();
                if((base <= g_int) && (g_int < base + io_apic.getMaxRedirEntry()))
                {
                        return &io_apic;
                }
        }
        return nullptr;
}

uint32 IOAPIC::findGSysIntForIRQ(uint8 irq)
{
        uint8 g_sys_int = irq;

        for(auto& entry : irq_source_override_list_)
        {
                if(irq == entry.irq_source)
                {
                        g_sys_int = entry.g_sys_int;
                        break;
                }
        }
        return g_sys_int;
}

IOAPIC* IOAPIC::findIOAPICforIRQ(uint8 irq)
{
        return findIOAPICforGlobalInterrupt(findGSysIntForIRQ(irq));
}


void IOAPIC::addIOAPIC(uint32 id, IOAPIC_MMIORegs* regs, uint32 g_sys_int_base)
{
        io_apic_list_.emplace_back(id, regs, g_sys_int_base);
}
