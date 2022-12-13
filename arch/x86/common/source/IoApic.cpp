#include "IoApic.h"
#include "ArchMemory.h"
#include "ArchMulticore.h"
#include "ArchInterrupts.h"
#include "8259.h"
#include "CPUID.h"
#include "debug.h"

IOAPIC::IOAPIC(uint32_t id, IOAPIC_MMIORegs* regs, uint32_t g_sys_int_base) :
    IrqDomain("I/O APIC", this),
    Device("I/O APIC"),
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

        for(auto& io_apic : IoApicList())
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

        PIC8259::enabled = false;

        IOAPIC_r_ID id = read<Register::ID>();
        IOAPIC_r_VER version = read<Register::VERSION>();
        max_redir_ = version.max_redir;
        debug(APIC, "IOAPIC id: %u, version: %#x, g_sys_ints: [%u, %u)\n", id.io_apic_id, version.version, getGlobalInterruptBase(), getGlobalInterruptBase() + getMaxRedirEntry());
        redir_entry_cache_.resize(max_redir_+1);

        setupIsaIrqMappings();
        initRedirections();
}

void IOAPIC::setupIsaIrqMappings()
{
    size_t ioapic_irq_start = getGlobalInterruptBase();
    size_t ioapic_irq_end = getGlobalInterruptBase() + getMaxRedirEntry();

    for (size_t isa_irq = ioapic_irq_start; isa_irq < ioapic_irq_end && isa_irq < 16; ++isa_irq)
    {
        // IRQ 2 is never raised (internal PIC cascade)
        if (isa_irq == 2)
            continue;

        auto [have_irq_override, ovr] = findSourceOverrideForIrq(isa_irq);
        auto g_sys_int = have_irq_override ? ovr.g_sys_int : isa_irq;
        ArchInterrupts::isaIrqDomain().irq(isa_irq).mapTo(*this, g_sys_int);
    }
}

void IOAPIC::initRedirections()
{
        for(uint32_t i = 0; i <= max_redir_; ++i)
        {
                auto g_sys_int = getGlobalInterruptBase() + i;
                IOAPIC_redir_entry r = readRedirEntry(i);

                r.interrupt_vector = Apic::IRQ_VECTOR_OFFSET + g_sys_int;

                auto [have_override, entry] = findSourceOverrideForGSysInt(g_sys_int);

                if(have_override)
                {
                    debug(APIC, "Found override for global system interrupt %2u -> IRQ SRC %u, trigger mode: %u, polarity: %u\n", entry.g_sys_int, entry.irq_source, entry.flags.trigger_mode, entry.flags.polarity);
                    r.interrupt_vector = Apic::IRQ_VECTOR_OFFSET + entry.irq_source;
                    r.polarity = (entry.flags.polarity == ACPI_MADT_POLARITY_ACTIVE_HIGH);
                    r.trigger_mode = (entry.flags.trigger_mode == ACPI_MADT_TRIGGER_LEVEL);
                    r.destination = cpu_lapic->apicId();
                }

                writeRedirEntry(i, r);
                auto target_cpu = SMP::cpu(r.destination);
                // Don't create an irq mapping if the irq source is connected to a different I/O APIC global system interrupt
                if (auto [have_override, ovr] = findSourceOverrideForIrq(g_sys_int); !have_override || ovr.irq_source == ovr.g_sys_int)
                {
                    irq(g_sys_int).mapTo(target_cpu->rootIrqDomain(), r.interrupt_vector);
                }
        }

        if(APIC & OUTPUT_ENABLED)
        {
                for(uint32_t i = 0; i <= max_redir_; ++i)
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

uint32_t IOAPIC::read(uint8_t offset)
{
        WithInterrupts i(false);
        uint32_t retval = 0;
        asm volatile("movl %[offset], %[io_reg_sel]\n"
                     "movl %[io_win], %[retval]\n"
                     :[io_reg_sel]"=m"(reg_vaddr_->io_reg_sel),
                      [retval]"=r"(retval)
                     :[offset]"r"((uint32)offset),
                      [io_win]"m"(reg_vaddr_->io_win));
        return retval;
}

void IOAPIC::write(uint8_t offset, uint32_t value)
{
        WithInterrupts i(false);
        asm volatile("movl %[offset], %[io_reg_sel]\n"
                     "movl %[value], %[io_win]\n"
                     :[io_reg_sel]"=m"(reg_vaddr_->io_reg_sel),
                      [io_win]"=m"(reg_vaddr_->io_win)
                     :[offset]"r"((uint32)offset),
                      [value]"r"(value));
}

uint8_t IOAPIC::redirEntryOffset(uint32_t entry_no)
{
        return (0x10 + 2 * entry_no);
}

IOAPIC::IOAPIC_redir_entry IOAPIC::readRedirEntry(uint32_t entry_no)
{
    debug(APIC, "IoApic, read redir entry %u\n", entry_no);
        assert(entry_no <= max_redir_);
        assert(entry_no < redir_entry_cache_.size());
        uint8_t offset = redirEntryOffset(entry_no);

        IOAPIC_redir_entry temp;
        temp.word_l = read(offset);
        temp.word_h = read(offset + 1);

        redir_entry_cache_[entry_no] = temp;

        return temp;
}

void IOAPIC::writeRedirEntry(uint32_t entry_no, const IOAPIC::IOAPIC_redir_entry& value)
{
        assert(entry_no <= max_redir_);
        assert(entry_no < redir_entry_cache_.size());

        redir_entry_cache_[entry_no] = value;

        uint8_t offset = redirEntryOffset(entry_no);

        write(offset + 1, value.word_h);
        write(offset, value.word_l);
}

uint32_t IOAPIC::getGlobalInterruptBase()
{
        return g_sys_int_base_;
}

uint32_t IOAPIC::getMaxRedirEntry()
{
        return max_redir_;
}


void IOAPIC::setGSysIntMask(uint32_t g_sys_int, bool value)
{
        debug(APIC, "Set G Sys Int %x mask: %u\n", g_sys_int, value);
        IOAPIC* io_apic = findIOAPICforGlobalInterrupt(g_sys_int);
        uint32_t entry_offset = g_sys_int - io_apic->getGlobalInterruptBase();
        IOAPIC_redir_entry r = io_apic->readRedirEntry(entry_offset);
        r.mask = (value ? 1: 0);
        io_apic->writeRedirEntry(entry_offset, r);
}

void IOAPIC::setIRQMask(uint32_t irq_num, bool value)
{
        debug(APIC, "Set IRQ %x mask: %u\n", irq_num, value);
        setGSysIntMask(findGSysIntForIRQ(irq_num), value);
}

IOAPIC* IOAPIC::findIOAPICforGlobalInterrupt(uint32_t g_int)
{
        for(auto& io_apic : IoApicList())
        {
                uint32_t base = io_apic.getGlobalInterruptBase();
                if((base <= g_int) && (g_int < base + io_apic.getMaxRedirEntry()))
                {
                    debugAdvanced(APIC, "Found IOAPIC for global interrupt %u: %p\n", g_int, &io_apic);

                    return &io_apic;
                }
        }

        debugAdvanced(APIC, "Couldn't find IOAPIC for global interrupt %u\n", g_int);

        return nullptr;
}

uint32_t IOAPIC::findGSysIntForIRQ(uint8_t irq)
{
        uint8_t g_sys_int = irq;

        for(auto& entry : IrqSourceOverrideList())
        {
                if(irq == entry.irq_source)
                {
                        g_sys_int = entry.g_sys_int;
                        break;
                }
        }

        debugAdvanced(APIC, "IRQ %u -> g sys int %u\n", irq, g_sys_int);
        return g_sys_int;
}

IOAPIC* IOAPIC::findIOAPICforIRQ(uint8_t irq)
{
    debugAdvanced(APIC, "Find IOAPIC for IRQ %u\n", irq);

    return findIOAPICforGlobalInterrupt(findGSysIntForIRQ(irq));
}

void IOAPIC::addIOAPIC(uint32_t id, IOAPIC_MMIORegs* regs, uint32_t g_sys_int_base)
{
    IoApicList().emplace_back(id, regs, g_sys_int_base);
}

void IOAPIC::addIRQSourceOverride(const MADTInterruptSourceOverride& entry)
{
    IrqSourceOverrideList().push_back(entry);
}

eastl::tuple<bool, MADTInterruptSourceOverride> IOAPIC::findSourceOverrideForGSysInt(uint8_t g_sys_int)
{
    for (auto o : IrqSourceOverrideList())
    {
        if (o.g_sys_int == g_sys_int)
            return {true, o};
    }
    return {false, {}};
}

eastl::tuple<bool, MADTInterruptSourceOverride> IOAPIC::findSourceOverrideForIrq(uint8_t irq)
{
    for (auto o : IrqSourceOverrideList())
    {
        if (o.irq_source == irq)
            return {true, o};
    }
    return {false, {}};
}

uint8_t IOAPIC::gSysIntToVector(uint8_t g_sys_int)
{
    assert(g_sys_int < redir_entry_cache_.size());
    return redir_entry_cache_[g_sys_int].interrupt_vector;
}

eastl::vector<IOAPIC>& IOAPIC::IoApicList()
{
    static eastl::vector<IOAPIC> io_apic_list_;
    return io_apic_list_;
}

eastl::vector<MADTInterruptSourceOverride>& IOAPIC::IrqSourceOverrideList()
{
    static eastl::vector<MADTInterruptSourceOverride> irq_source_override_list_;
    return irq_source_override_list_;
}

bool IOAPIC::mask(irqnum_t irq, bool mask)
{
    debug(APIC, "IoApic, mask Irq %zu = %u\n", irq, mask);
    setIRQMask(irq, mask);
    return true;
}

bool IOAPIC::irqStart(irqnum_t irq)
{
    debugAdvanced(APIC, "IoApic, start Irq %zu\n", irq);
    ++pending_EOIs;
    return true;
}

bool IOAPIC::ack(irqnum_t irq)
{
    debugAdvanced(APIC, "IoApic, ack Irq %zu\n", irq);
    --pending_EOIs;
    cpu_lapic->sendEOI(gSysIntToVector(irq));
    return true;
}

void IoApicDriver::doDeviceDetection()
{
    if (cpu_features.cpuHasFeature(CpuFeatures::APIC))
    {
        IOAPIC::initAll();
        for (auto& ioapic : IOAPIC::IoApicList())
        {
            bindDevice(ioapic);
        }
    }
}