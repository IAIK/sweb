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

IOAPIC IO_APIC;

bool LocalAPIC::exists = false;
LocalAPICRegisters* LocalAPIC::reg_paddr_ = nullptr;
LocalAPICRegisters* LocalAPIC::reg_vaddr_ = nullptr;
ustl::vector<MADTProcLocalAPIC> LocalAPIC::local_apic_list_{};

bool IOAPIC::exists      = false;
bool IOAPIC::initialized = false;

extern volatile size_t outstanding_EOIs;

LocalAPIC::LocalAPIC() :
        outstanding_EOIs_(0)
{
  debug(APIC, "LocalAPIC ctor\n");
  if(LocalAPIC::exists)
  {
    if((size_t)lapic.reg_vaddr_ != APIC_VADDR)
    {
            lapic.mapAt(APIC_VADDR);
    }
    lapic.init();
  }
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
  //debug(APIC, "CPU %zu, Sending EOI for %zx\n", ArchMulticore::getCpuID(), num);
  reg_vaddr_->eoi = 0;
}


void LocalAPIC::mapAt(size_t addr)
{
  assert(addr);
  assert(exists);

  debug(APIC, "Map local APIC at phys %p to %zx\n", reg_paddr_, addr);

  ArchMemory::mapKernelPage(addr/PAGE_SIZE, ((size_t)reg_paddr_)/PAGE_SIZE, true, true);
  reg_vaddr_ = (LocalAPICRegisters*)addr;
}


void LocalAPIC::init()
{
  if(!isInitialized())
  {
    debug(APIC, "Initializing Local APIC %x\n", getID());
    setSpuriousInterruptNumber(100);
    initTimer();
    enable(true);
    initialized_ = true;
  }
  else
  {
    debug(APIC, "Local APIC %x is already initialized. Skipping re-initialization\n", getID());
  }
}

bool LocalAPIC::isInitialized()
{
  return initialized_;
}

void LocalAPIC::enable(bool enable)
{
  debug(APIC, "%s APIC\n", (enable ? "Enabling" : "Disabling"));
  uint32* ptr = (uint32*)&reg_vaddr_->s_int_vect;
  uint32 temp = *ptr;
  ((LocalAPIC_SpuriousInterruptVector*)&temp)->enable = (enable ? 1 : 0);
  *ptr = temp;
}

void LocalAPIC::initTimer() volatile
{
        //reg_vaddr_->lvt_timer.setVector(90);
        reg_vaddr_->lvt_timer.setVector(0x20);
        reg_vaddr_->lvt_timer.setMode(1);
        reg_vaddr_->lvt_timer.setMask(true);
        reg_vaddr_->timer_divide_config.setTimerDivisor(16);
        setTimerPeriod(0x500000);
        use_apic_timer_ = true;
}

bool LocalAPIC::usingAPICTimer()
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

void LocalAPIC_LVT_TimerRegister::setMask(bool mask) volatile
{
        debug(APIC, "Set timer mask %u\n", mask);
        uint32 temp = *(uint32*)this;
        ((LocalAPIC_LVT_TimerRegister*)&temp)->mask = (mask ? 1 : 0);
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
        uint8 byte_offset = num/8;
        uint8 bit_offset = num % 8;

        return reg_vaddr_->IRR[byte_offset].irr & (1 << bit_offset);
}

bool LocalAPIC::checkISR(uint8 num) volatile
{
        uint8 byte_offset = num/8;
        uint8 bit_offset = num % 8;

        return reg_vaddr_->ISR[byte_offset].isr & (1 << bit_offset);
}

uint32 LocalAPIC::getID() volatile
{
        LocalAPIC_IDRegister id;
        *(uint32*)&id = *(uint32*)&reg_vaddr_->local_apic_id;
        return id.id;
}

static volatile uint8 delay = 0;

extern "C" void PIT_delay_IRQ();
void __PIT_delay_IRQ()
{
        __asm__ __volatile__(".global PIT_delay_IRQ\n"
                             "PIT_delay_IRQ:\n");
        __asm__ __volatile__("movb $1, %[delay]\n"
                             :[delay]"=m"(delay));
        __asm__ __volatile__("iretq\n");
}
extern "C" void arch_irqHandler_0();

void LocalAPIC::startAPs(size_t entry_addr) volatile
{
        debug(A_MULTICORE, "Sending init IPI to AP local APICs, AP entry function: %zx\n", entry_addr);

        LocalAPIC_InterruptCommandRegisterHigh v_high{};
        v_high.destination = 1;


        LocalAPIC_InterruptCommandRegisterLow v_low{};
        v_low.vector = 0;
        v_low.delivery_mode = 5; // INIT
        v_low.destination_mode = 0;
        v_low.level = 1;
        v_low.trigger_mode = 0;
        //v_low.destination_shorthand = 3; // Send to all excluding self
        v_low.destination_shorthand = 0; // Send to cpu specified in ICR high

        *(volatile uint32*)&reg_vaddr_->ICR_high  = *(uint32*)&v_high;
        *(volatile uint32*)&reg_vaddr_->ICR_low  = *(uint32*)&v_low;

        debug(A_MULTICORE, "Start delay 1\n");
        // 10ms delay

        PIT::PITCommandRegister pit_command{};
        pit_command.bcd_mode = 0;
        pit_command.operating_mode = 0; // oneshot
        pit_command.access_mode = 3; // send low + high byte of reload value/divisor
        pit_command.channel = 0;

        __attribute__((unused)) InterruptGateDesc temp_irq0_descriptor = InterruptUtils::idt[0x20];

        InterruptUtils::idt[0x20].setOffset((size_t)&PIT_delay_IRQ);

        if(IOAPIC::initialized)
        {
                IO_APIC.setIRQMask(2, false);
        }
        else
        {
                PIC8259::outstanding_EOIs_++;
                PIC8259::enableIRQ(0);
        }
        ArchInterrupts::enableInterrupts();
        PIT::init(pit_command.value, 1193182 / 100);
        while(!delay);
        ArchInterrupts::disableInterrupts();
        if(IOAPIC::initialized)
        {
                IO_APIC.setIRQMask(2, true);
                lapic.sendEOI(0x20);
        }
        else
        {
                PIC8259::disableIRQ(0);
                PIC8259::sendEOI(0);
        }

        delay = 0;

        debug(A_MULTICORE, "End delay 1\n");

        assert((entry_addr % PAGE_SIZE) == 0);
        assert((entry_addr/PAGE_SIZE) <= 0xFF);
        v_low.vector = entry_addr/PAGE_SIZE;
        v_low.delivery_mode = 6; // SIPI

        *(volatile uint32*)&reg_vaddr_->ICR_high  = *(uint32*)&v_high;
        *(volatile uint32*)&reg_vaddr_->ICR_low  = *(uint32*)&v_low;

        ArchMulticore::cpus_started_ = true;

        // 200us delay
        debug(A_MULTICORE, "Start delay 2\n");

        if(IOAPIC::initialized)
        {
                IO_APIC.setIRQMask(2, false);
        }
        else
        {
                PIC8259::outstanding_EOIs_++;
                PIC8259::enableIRQ(0);
        }

        ArchInterrupts::enableInterrupts();
        PIT::init(pit_command.value, 1193182 / 5000);
        while(!delay);
        ArchInterrupts::disableInterrupts();
        if(IOAPIC::initialized)
        {
                IO_APIC.setIRQMask(2, true);
                lapic.sendEOI(0x20);
        }
        else
        {
                PIC8259::disableIRQ(0);
                PIC8259::sendEOI(0);
        }

        delay = 0;

        debug(A_MULTICORE, "End delay 2\n");

        *(volatile uint32*)&reg_vaddr_->ICR_high  = *(uint32*)&v_high;
        *(volatile uint32*)&reg_vaddr_->ICR_low  = *(uint32*)&v_low;

        // Wait another 10ms to give APs time for initialization
        if(IOAPIC::initialized)
        {
                IO_APIC.setIRQMask(2, false);
        }
        else
        {
                PIC8259::outstanding_EOIs_++;
                PIC8259::enableIRQ(0);
        }
        ArchInterrupts::enableInterrupts();
        PIT::init(pit_command.value, 1193182 / 100);
        while(!delay);
        ArchInterrupts::disableInterrupts();
        if(IOAPIC::initialized)
        {
                IO_APIC.setIRQMask(2, true);
                lapic.sendEOI(0x20);
        }
        else
        {
                PIC8259::disableIRQ(0);
                PIC8259::sendEOI(0);
        }


        InterruptUtils::idt[0x20] = temp_irq0_descriptor;

        debug(A_MULTICORE, "Finished sending IPI to AP local APICs\n");
}


void LocalAPIC::sendIPI(uint8 vector) volatile
{
        debug(APIC, "Sending IPI, vector: %x\n", vector);
        LocalAPIC_InterruptCommandRegisterHigh v_high{};
        v_high.destination = 1;

        LocalAPIC_InterruptCommandRegisterLow v_low{};
        v_low.vector = vector;
        v_low.delivery_mode = 0; // normal IPI
        v_low.destination_mode = 0; // physical
        v_low.level = 1;
        v_low.trigger_mode = 0;
        //v_low.destination_shorthand = 3; // Send to all excluding self
        v_low.destination_shorthand = 2; // Send to all
        //v_low.destination_shorthand = 0; // Send to cpu specified in ICR high


        *(volatile uint32*)&reg_vaddr_->ICR_high  = *(uint32*)&v_high;
        *(volatile uint32*)&reg_vaddr_->ICR_low  = *(uint32*)&v_low;
}



IOAPIC::IOAPIC() :
        reg_paddr_(nullptr),
        reg_vaddr_(nullptr)
{}

IOAPIC::IOAPIC(uint32 id, IOAPIC_MMIORegs* regs, uint32 g_sys_int_base) :
        reg_paddr_(regs),
        reg_vaddr_(regs),
        id_(id),
        g_sys_int_base_(g_sys_int_base)
{
        debug(APIC, "IOAPIC %x at phys %p, g_sys_int_base: %x, version: \n", id_, reg_paddr_, g_sys_int_base_);
        assert(reg_paddr_);
        exists = true;
}

void IOAPIC::init()
{
        IOAPIC_r_ID id;
        IOAPIC_r_VER version;
        id.word = IO_APIC.read(IOAPICID);
        version.word = IO_APIC.read(IOAPICVER);
        max_redir_ = version.max_redir;
        debug(APIC, "IOAPIC id: %x\n", id.io_apic_id);
        debug(APIC, "IOAPIC version: %x\n", version.version);
        debug(APIC, "IOAPIC max redir: %x\n", getMaxRedirEntry());
        debug(APIC, "IOAPIC g_sys_int base: %x\n", getGlobalInterruptBase());

        IO_APIC.initRedirections();
        IOAPIC::initialized = true;
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
                                debug(APIC, "Found override for IRQ %2u -> %u, trigger mode: %u, polarity: %u\n", entry.g_sys_int, entry.irq_source, entry.flags.trigger_mode, entry.flags.polarity);
                                r.interrupt_vector = IRQ_OFFSET + entry.irq_source;
                                r.polarity = (entry.flags.polarity == ACPI_MADT_POLARITY_ACTIVE_HIGH);
                                r.trigger_mode = (entry.flags.trigger_mode == ACPI_MADT_TRIGGER_LEVEL);
                                r.destination = lapic.getID();
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
}


void IOAPIC::mapAt(size_t addr)
{
  debug(APIC, "Map IOAPIC at phys %p to %zx\n", reg_paddr_, addr);
  assert(addr);

  ArchMemory::mapKernelPage(addr/PAGE_SIZE, ((size_t)reg_paddr_)/PAGE_SIZE, true, true);
  reg_vaddr_ = (IOAPIC_MMIORegs*)addr;
}

uint32 IOAPIC::read(uint8 offset)
{
        reg_vaddr_->io_reg_sel = offset;
        return reg_vaddr_->io_win;
}

void IOAPIC::write(uint8 offset, uint32 value)
{
        reg_vaddr_->io_reg_sel = offset;
        reg_vaddr_->io_win = value;
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

        write(offset, value.word_l);
        write(offset + 1, value.word_h);
}

uint32 IOAPIC::getGlobalInterruptBase()
{
        return g_sys_int_base_;
}

uint32 IOAPIC::getMaxRedirEntry()
{
        return max_redir_;
}


void IOAPIC::setIRQMask(uint32 irq_num, bool value)
{
        debug(APIC, "Set IRQ %x mask: %u\n", irq_num, value);
        IOAPIC_redir_entry r = readRedirEntry(irq_num);
        r.mask = (value ? 1: 0);
        writeRedirEntry(irq_num, r);
}
