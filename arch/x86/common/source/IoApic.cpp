#include "IoApic.h"
#include "ArchMemory.h"
#include "ArchMulticore.h"
#include "ArchInterrupts.h"
#include "debug.h"

eastl::vector<MADTInterruptSourceOverride> IOAPIC::irq_source_override_list_{};
eastl::vector<IOAPIC> IOAPIC::io_apic_list_{};

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

        // TODO: Proper address assignment for ioapic on x86
        pointer ioapic_vaddr = ArchMemory::getIdentAddressOfPPN(0) - PAGE_SIZE*3;

        for(auto& io_apic : io_apic_list_)
        {
            io_apic.mapAt(ioapic_vaddr - io_apic.id_*PAGE_SIZE);
            assert((size_t)io_apic.reg_vaddr_ >= USER_BREAK);
            io_apic.init();
        }
}

void IOAPIC::init()
{
        //TODO: Reading from IO APIC id/version registers on x86_32 returns 0
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
                                r.destination = cpu_lapic->Id();
                                goto write_entry;
                        }
                }

                r.interrupt_vector = getGlobalInterruptBase() + IRQ_OFFSET + i;

        write_entry:
                writeRedirEntry(i, r);
        }

        if(APIC & OUTPUT_ENABLED)
        {
                for(uint32 i = 0; i <= max_redir_; ++i)
                {
                        IOAPIC_redir_entry r = readRedirEntry(i);
                        debug(APIC, "IOAPIC redir entry: IRQ %2u -> vector %u, dest mode: %u, dest APIC: %u, mask: %u, pol: %u, trig: %u\n", getGlobalInterruptBase() + i, r.interrupt_vector, r.destination_mode, r.destination, r.mask, r.polarity, r.trigger_mode);
                }
        }
        debug(APIC, "IO APIC redirections initialized\n");
}


void IOAPIC::mapAt(size_t addr)
{
  debug(APIC, "Map IOAPIC %u at phys %p to %p\n", id_, reg_paddr_, (void*)addr);
  assert(addr);

  assert(ArchMemory::mapKernelPage(addr/PAGE_SIZE, ((size_t)reg_paddr_)/PAGE_SIZE, true, true));
  reg_vaddr_ = (IOAPIC_MMIORegs*)addr;
}

uint32 IOAPIC::read(uint8 offset)
{
    if(APIC & OUTPUT_ADVANCED){
        debug(APIC, "IO APIC read from registers %p with offset %#x\n", reg_vaddr_, offset);
    }

        WithInterrupts i(false);
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
        WithInterrupts i(false);
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
                    if (APIC & OUTPUT_ADVANCED)
                        debug(APIC, "Found IOAPIC for global interrupt %u: %p\n", g_int, &io_apic);

                    return &io_apic;
                }
        }
        if (APIC & OUTPUT_ADVANCED)
            debug(APIC, "Couldn't find IOAPIC for global interrupt %u\n", g_int);

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
        if(APIC & OUTPUT_ADVANCED){
            debug(APIC, "IRQ %u -> g sys int %u\n", irq, g_sys_int);
        }
        return g_sys_int;
}

IOAPIC* IOAPIC::findIOAPICforIRQ(uint8 irq)
{
    if(APIC & OUTPUT_ADVANCED){
        debug(APIC, "Find IOAPIC for IRQ %u\n", irq);
    }

    return findIOAPICforGlobalInterrupt(findGSysIntForIRQ(irq));
}

void IOAPIC::addIOAPIC(uint32 id, IOAPIC_MMIORegs* regs, uint32 g_sys_int_base)
{
    io_apic_list_.emplace_back(id, regs, g_sys_int_base);
}

void IOAPIC::addIRQSourceOverride(const MADTInterruptSourceOverride& entry)
{
    irq_source_override_list_.push_back(entry);
}
