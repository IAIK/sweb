#include "APIC.h"
#include "debug.h"
#include "assert.h"
#include "ArchMemory.h"
#include "PageManager.h"
#include "new.h"

LocalAPIC* local_APIC = nullptr;

IOAPIC IO_APIC;
bool IOAPIC::initialized = false;

LocalAPIC* initAPIC(ACPI_MADTHeader* madt)
{
  debug(APIC, "initAPIC, MADT: %p\n", madt);
  local_APIC = (LocalAPIC*)(size_t)madt->ext_header.local_apic_addr;
  debug(APIC, "MADT: Local APIC addr: %p, flags: %x\n", local_APIC, madt->ext_header.flags);

  MADTEntryDescriptor* madt_entry = (MADTEntryDescriptor*)(madt + 1);
  while((size_t)madt_entry < (size_t)madt + madt->std_header.Length)
  {
    switch(madt_entry->type)
    {
    case 0:
    {
      MADTProcLocalAPIC* entry = (MADTProcLocalAPIC*)(madt_entry + 1);
      debug(APIC, "[%p] Processor local APIC, ACPI Processor ID: %4x, APIC ID: %4x, flags: %x\n", entry, entry->proc_id, entry->apic_id, entry->flags);
      break;
    }
    case 1:
    {
      MADT_IO_APIC* entry = (MADT_IO_APIC*)(madt_entry + 1);
      debug(APIC, "[%p] I/O APIC, id: %x, address: %x, g_sys_int base: %x\n", entry, entry->id, entry->address, entry->global_system_interrupt_base);
      if(!IOAPIC::initialized)
      {
              new (&IO_APIC) IOAPIC(entry->id, (IOAPIC::IOAPIC_MMIORegs*)(size_t)entry->address, entry->global_system_interrupt_base);
      }
      break;
    }
    case 2:
    {
      MADTInterruptSourceOverride* entry = (MADTInterruptSourceOverride*)(madt_entry + 1);
      debug(APIC, "[%p] Interrupt Source Override, bus_source: %x, irq_source: %3x, g_sys_int: %3x, flags: %3x\n", entry, entry->bus_source, entry->irq_source, entry->global_system_interrupt, entry->flags);
      break;
    }
    case 4:
    {
      MADTNonMaskableInterrupts* entry = (MADTNonMaskableInterrupts*)(madt_entry + 1);
      debug(APIC, "[%p] Non maskable interrupts, proc_id: %x, flags: %x, lint_num: %x\n", entry, entry->processor_id, entry->flags, entry->lint_num);
      break;
    }
    case 5:
    {
      MADTLocalAPICAddressOverride* entry = (MADTLocalAPICAddressOverride*)(madt_entry + 1);
      debug(APIC, "[%p] Local APIC address override, addr: %zx\n", entry, entry->local_apic_addr);
      break;
    }
    default:
      debug(APIC, "[%p] Unknown MADT entry type %x\n", madt_entry + 1, madt_entry->type);
      break;
    }
    madt_entry = (MADTEntryDescriptor*)((size_t)madt_entry + madt_entry->length);
  }

  return local_APIC;
}


void LocalAPIC::sendEOI(size_t num)
{
  debug(APIC, "Sending EOI for %zx\n", num);
  registers.eoi = 0;
}


void LocalAPIC::mapAt(size_t addr)
{
  assert(local_APIC);

  debug(APIC, "Map local APIC at phys %p to %zx\n", local_APIC, addr);
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
        m.pd[m.pdi].pt.write_through = 1;
        m.pd[m.pdi].pt.cache_disabled = 1;
        m.pd[m.pdi].pt.present = 1;
        m.pt = (PageTableEntry*)ArchMemory::getIdentAddressOfPPN(pt_ppn);
        debug(APIC, "Mapped PDI %zx to ppn: %zx\n", m.pdi, pt_ppn);
      }
    }
  }

  ArchMemory::mapKernelPage(addr/PAGE_SIZE, ((size_t)local_APIC)/PAGE_SIZE);
  local_APIC = (LocalAPIC*)addr;
}


void LocalAPIC::enable(bool enable)
{
        debug(APIC, "%s APIC\n", (enable ? "Enabling" : "Disabling"));
        uint32* ptr = (uint32*)&registers.s_int_vect;
        uint32 temp = *ptr;
        ((LocalAPIC_SpuriousInterruptVector*)&temp)->enable = (enable ? 1 : 0);
        *ptr = temp;
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
                if(getGlobalInterruptBase() + i == 2)
                {
                        // Hardcoded interrupt source override for timer (2 -> 0)
                        // TODO: Dynamically read interrupt source override tables in ACPI->MADT
                        r.interrupt_vector = getGlobalInterruptBase() + IRQ_OFFSET + 0;
                }
                else
                {
                        r.interrupt_vector = getGlobalInterruptBase() + IRQ_OFFSET + i;
                }
                writeRedirEntry(i, r);
        }

        for(uint32 i = 0; i <= max_redir_; ++i)
        {
                IOAPIC_redir_entry r = readRedirEntry(i);
                debug(APIC, "IOAPIC redir entry %u -> vector %u, destination: %u, mask: %u\n", getGlobalInterruptBase() + i, r.interrupt_vector, r.destination, r.mask);
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
        m.pd[m.pdi].pt.write_through = 1;
        m.pd[m.pdi].pt.cache_disabled = 1;
        m.pd[m.pdi].pt.present = 1;
        m.pt = (PageTableEntry*)ArchMemory::getIdentAddressOfPPN(pt_ppn);
        debug(APIC, "Mapped PDI %zx to ppn: %zx\n", m.pdi, pt_ppn);
      }
    }
  }

  ArchMemory::mapKernelPage((size_t)addr/PAGE_SIZE, ((size_t)reg_paddr_)/PAGE_SIZE);
  reg_vaddr_ = (IOAPIC_MMIORegs*)addr;
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
        registers.init_timer_count = count;
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

        return registers.IRR[byte_offset].irr & (1 << bit_offset);
}

bool LocalAPIC::checkISR(uint8 num) volatile
{
        uint8 byte_offset = num/8;
        uint8 bit_offset = num % 8;

        return registers.ISR[byte_offset].isr & (1 << bit_offset);
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
