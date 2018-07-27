#include "APIC.h"
#include "debug.h"
#include "assert.h"
#include "ArchMemory.h"
#include "PageManager.h"
#include "new.h"
#include "kstring.h"

LocalAPIC local_APIC;
IOAPIC IO_APIC;

bool IOAPIC::initialized    = false;
bool LocalAPIC::initialized = false;

LocalAPIC::LocalAPIC() :
        reg_paddr_(nullptr),
        reg_vaddr_(nullptr)
{
}

LocalAPIC::LocalAPIC(ACPI_MADTHeader* madt) :
        reg_paddr_((LocalAPICRegisters*)(size_t)madt->ext_header.local_apic_addr),
        reg_vaddr_((LocalAPICRegisters*)(size_t)madt->ext_header.local_apic_addr)
{
  debug(APIC, "Local APIC at phys %p, flags: %x\n", reg_paddr_, madt->ext_header.flags);
  assert(reg_paddr_);
  initialized = true;
}

void LocalAPIC::sendEOI(size_t num)
{
  debug(APIC, "Sending EOI for %zx\n", num);
  reg_vaddr_->eoi = 0;
}


void LocalAPIC::mapAt(size_t addr)
{
  assert(initialized);

  debug(APIC, "Map local APIC at phys %p to %zx\n", reg_paddr_, addr);
  auto m = ArchMemory::resolveMapping(((uint64) VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4) / PAGE_SIZE), addr/PAGE_SIZE);
  if(m.pml4[m.pml4i].present)
  {
    debug(APIC, "PML4i %zx present, ppn: %zx\n", m.pml4i, m.pml4[m.pml4i].page_ppn);
    if(m.pdpt[m.pdpti].pd.present)
    {
      debug(APIC, "PDPTI %zx present, ppn: %zx\n", m.pdpti, m.pdpt[m.pdpti].pd.page_ppn);
      if(m.pd[m.pdi].pt.present)
      {
        debug(APIC, "PDI %zx present, ppn: %zx\n", m.pdi, m.pd[m.pdi].pt.page_ppn);
        if(m.pt[m.pti].present)
        {
          debug(APIC, "PTI %zx present, ppn: %zx\n", m.pti, m.pt[m.pti].page_ppn);
        }
      }
      else
      {
        size_t pt_ppn = PageManager::instance()->allocPPN();
        m.pd[m.pdi].pt.page_ppn = pt_ppn;
        m.pd[m.pdi].pt.writeable = 1;
        m.pd[m.pdi].pt.present = 1;
        m.pt = (PageTableEntry*)ArchMemory::getIdentAddressOfPPN(pt_ppn);
        debug(APIC, "Mapped PDI %zx to ppn: %zx\n", m.pdi, pt_ppn);
      }
    }
  }

  ArchMemory::mapKernelPage(addr/PAGE_SIZE, ((size_t)reg_paddr_)/PAGE_SIZE);
  auto m2 = ArchMemory::resolveMapping(((uint64) VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4) / PAGE_SIZE), addr/PAGE_SIZE);
  assert(m2.pt);
  m2.pt[m2.pti].write_through = 1;
  m2.pt[m2.pti].cache_disabled = 1;
  reg_vaddr_ = (LocalAPICRegisters*)addr;
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
        reg_vaddr_->lvt_timer.setVector(0x20);
        reg_vaddr_->lvt_timer.setMode(1);
        reg_vaddr_->lvt_timer.setMask(true);
        reg_vaddr_->timer_divide_config.setTimerDivisor(16);
        setTimerPeriod(0x1000000);
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

extern char apstartup_text_begin;
extern char apstartup_text_end;
extern "C" void apstartup();
struct GDT32Ptr
{
    uint16 limit;
    uint32 addr;
}__attribute__((__packed__));
extern GDT32Ptr ap_gdt32_ptr;
extern SegmentDescriptor gdt[7];
extern SegmentDescriptor ap_gdt32[7];

extern uint32 ap_kernel_cr3;
extern char ap_pml4[PAGE_SIZE];

char dummy_stack[0x1000];

extern "C" void __apstartup64()
{
        __asm__ __volatile__(".global apstartup64\n"
                             "apstartup64:\n");
        __asm__ __volatile__("movq %[stack], %%rsp\n"::[stack]"i"(&dummy_stack));
        debug(APIC, "AP startup 64\n");
        __asm__ __volatile__("hlt\n");
}

void LocalAPIC::startAPs() volatile
{
        debug(APIC, "Sending init IPI to AP local APICs, ICR low: %p, ICR high: %p\n", &reg_vaddr_->ICR_low, &reg_vaddr_->ICR_high);

        size_t apstartup_size = (size_t)(&apstartup_text_end - &apstartup_text_begin);
        debug(APIC, "apstartup_text_begin: %p, apstartup_text_end: %p, size: %zx\n", &apstartup_text_begin, &apstartup_text_end, apstartup_size);

        pointer apstartup_phys = (pointer)VIRTUAL_TO_PHYSICAL_BOOT(AP_STARTUP_PADDR);
        debug(APIC, "apstartup %p, phys: %zx\n", &apstartup, apstartup_phys);
        pointer paddr0 = ArchMemory::getIdentAddress(AP_STARTUP_PADDR);

        auto m = ArchMemory::resolveMapping(((uint64) VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4) / PAGE_SIZE), paddr0/PAGE_SIZE);
        debug(APIC, "paddr0 ppn: %zx\n", m.page_ppn);
        assert(m.page_ppn == AP_STARTUP_PADDR/PAGE_SIZE);

        debug(APIC, "AP GDT32: %p\n", &ap_gdt32);
        memcpy(&ap_gdt32, &gdt, sizeof(ap_gdt32));


        debug(APIC, "AP GDT32 PTR: %p\n", &ap_gdt32_ptr);
        ap_gdt32_ptr.addr = AP_STARTUP_PADDR + ((size_t)&ap_gdt32 - (size_t)&apstartup_text_begin);
        ap_gdt32_ptr.limit = sizeof(ap_gdt32) - 1;
        debug(APIC, "AP GDT32 PTR addr: %x\n", ap_gdt32_ptr.addr);
        debug(APIC, "AP GDT32 PTR limit: %x\n", ap_gdt32_ptr.limit);

        debug(APIC, "AP kernel PML4: %zx\n", (size_t)&ap_pml4);
        memcpy(&ap_pml4, &kernel_page_map_level_4, sizeof(ap_pml4));

        debug(APIC, "AP kernel &CR3: %p\n", &ap_kernel_cr3);
        ap_kernel_cr3 = (size_t)VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4);
        debug(APIC, "AP kernel CR3 set to: %x (kernel pml4 = %p)\n", ap_kernel_cr3, kernel_page_map_level_4);


        debug(APIC, "Copying apstartup from virt [%p,%p] -> %p (phys: %zx), size: %zx\n", (void*)&apstartup_text_begin, (void*)&apstartup_text_end, (void*)paddr0, (size_t)AP_STARTUP_PADDR, (size_t)(&apstartup_text_end - &apstartup_text_begin));
        memcpy((void*)paddr0, (void*)&apstartup_text_begin, apstartup_size);
        assert(memcmp((void*)paddr0, (void*)&apstartup_text_begin, apstartup_size) == 0);

        size_t ap_gdt32_offset = (size_t)&ap_gdt32 - (size_t)&apstartup_text_begin;
        debug(APIC, "Copying AP GDT from virt %p -> %p (phys: %zx)\n", &ap_gdt32, (char*)paddr0 + ap_gdt32_offset, (size_t)AP_STARTUP_PADDR + ap_gdt32_offset);
        memcpy((char*)paddr0 + ap_gdt32_offset, &ap_gdt32, sizeof(ap_gdt32));


        LocalAPIC_InterruptCommandRegisterLow v_low{};
        v_low.vector = 0;
        v_low.delivery_mode = 5;
        v_low.destination_mode = 0;
        v_low.level = 1;
        v_low.trigger_mode = 0;
        v_low.destination_shorthand = 3;

        *(volatile uint32*)&reg_vaddr_->ICR_low  = *(uint32*)&v_low;

        // TODO: 10ms delay here
        debug(APIC, "Start delay 1\n");
        for(size_t i = 0; i < 0xFFFFFF; ++i)
        {
        }
        debug(APIC, "End delay 1\n");

        assert((AP_STARTUP_PADDR % PAGE_SIZE) == 0);
        assert((AP_STARTUP_PADDR/PAGE_SIZE) <= 0xFF);
        v_low.vector = AP_STARTUP_PADDR/PAGE_SIZE;
        v_low.delivery_mode = 6;

        *(volatile uint32*)&reg_vaddr_->ICR_low  = *(uint32*)&v_low;

        // TODO: 200us delay here
        debug(APIC, "Start delay 2\n");
        for(size_t i = 0; i < 0xFFFFF; ++i)
        {
        }
        debug(APIC, "End delay 2\n");

        *(volatile uint32*)&reg_vaddr_->ICR_low  = *(uint32*)&v_low;

        for(size_t i = 0; i < 0xFFFFFFF; ++i)
        {
        }

        debug(APIC, "Finished sending IPI to AP local APICs\n");
        //while(1);
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
        debug(APIC, "IOAPIC %x at phys %p, g_sys_int_base: %x\n", id_, reg_paddr_, g_sys_int_base_);
        assert(reg_paddr_);
        initialized = true;
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


void IOAPIC::mapAt(void* addr)
{
  debug(APIC, "Map IOAPIC at phys %p to %p\n", reg_paddr_, addr);
  assert(addr);
  auto m = ArchMemory::resolveMapping(((uint64) VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4) / PAGE_SIZE), (size_t)addr/PAGE_SIZE);
  if(m.pml4[m.pml4i].present)
  {
    debug(APIC, "PML4i %zx present, ppn: %zx\n", m.pml4i, m.pml4[m.pml4i].page_ppn);
    if(m.pdpt[m.pdpti].pd.present)
    {
      debug(APIC, "PDPTI %zx present, ppn: %zx\n", m.pdpti, m.pdpt[m.pdpti].pd.page_ppn);
      if(m.pd[m.pdi].pt.present)
      {
        debug(APIC, "PDI %zx present, ppn: %zx\n", m.pdi, m.pd[m.pdi].pt.page_ppn);
        if(m.pt[m.pti].present)
        {
          debug(APIC, "PTI %zx present, ppn: %zx\n", m.pti, m.pt[m.pti].page_ppn);
        }
      }
      else
      {
        size_t pt_ppn = PageManager::instance()->allocPPN();
        m.pd[m.pdi].pt.page_ppn = pt_ppn;
        m.pd[m.pdi].pt.writeable = 1;
        m.pd[m.pdi].pt.present = 1;
        m.pt = (PageTableEntry*)ArchMemory::getIdentAddressOfPPN(pt_ppn);
        debug(APIC, "Mapped PDI %zx to ppn: %zx\n", m.pdi, pt_ppn);
      }
    }
  }

  ArchMemory::mapKernelPage((size_t)addr/PAGE_SIZE, ((size_t)reg_paddr_)/PAGE_SIZE);
  auto m2 = ArchMemory::resolveMapping(((uint64) VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4) / PAGE_SIZE), (size_t)addr/PAGE_SIZE);
  assert(m2.pt);
  m2.pt[m2.pti].write_through = 1;
  m2.pt[m2.pti].cache_disabled = 1;
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
