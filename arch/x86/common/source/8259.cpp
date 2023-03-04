#include "8259.h"
#include "ports.h"
#include "debug.h"
#include "ArchInterrupts.h"
#include "APIC.h"
#include "assert.h"

bool PIC8259::exists = true;
bool PIC8259::enabled = true;
size_t PIC8259::outstanding_EOIs_ = 0;

uint16 PIC8259::cached_mask = 0xFFFF;

#define PIC_ICW1_INIT   0x11
#define PIC_ICW2_OFFSET 0x20 // offset for IRQ -> interrupt vector mapping (i.e. IRQ 0 is mapped to vector 32)
#define PIC_ICW4_8086   0x01

#define PIC_EOI 0x20

PIC8259::PIC8259() :
    IrqDomain("PIC 8259", 16, this),
    Device("PIC 8259")
{
}

PIC8259& PIC8259::instance()
{
    static PIC8259 pic;
    return pic;
}

void PIC8259::init()
{
    PIC8259::initialise8259s();

    if (enabled)
    {
        setupIrqMappings();
    }
}

void PIC8259::setupIrqMappings()
{
    for (size_t isa_irq = 0; isa_irq < 16; ++isa_irq)
    {
        // There is no ISA IRQ 2 (used for pic cascade)
        if (isa_irq == 2)
            continue;

        // Map ISA interrupts to PIC (if not already mapped to I/O APIC)
        IrqInfo* irq_info = ArchInterrupts::isaIrqDomain().irqInfo(isa_irq);
        if (!irq_info || !irq_info->map_to.domain)
        {
            ArchInterrupts::isaIrqDomain().irq(isa_irq).mapTo(instance(), isa_irq);
        }
    }

    for (size_t i = 0; i < 16; ++i)
    {
        // IRQ 2 is never raised (internal PIC cascade)
        if (i == 2)
            continue;

        irq(i).mapTo(*cpu_root_irq_domain_, Apic::IRQ_VECTOR_OFFSET + i);
    }
}

bool PIC8259::mask(irqnum_t irq, bool mask)
{
    assert(irq < 16);
    if (mask)
        disableIRQ(irq);
    else
        enableIRQ(irq);
    return true;
}

bool PIC8259::isMasked(irqnum_t irqnum)
{
    return !isIRQEnabled(irqnum);
}

bool PIC8259::irqStart([[maybe_unused]]irqnum_t irq)
{
    ++pending_EOIs;
    ++outstanding_EOIs_;
    return true;
}

bool PIC8259::ack(irqnum_t irq)
{
    assert(irq < 16);
    --pending_EOIs;
    sendEOI(irq);
    return true;
}

void PIC8259::initialise8259s()
{
    // https://wiki.osdev.org/8259_PIC#Initialisation
    debug(PIC_8259, "Init 8259 Programmable Interrupt Controller\n");
    PIC_1_CONTROL_PORT::write(PIC_ICW1_INIT); /* ICW1 */
    PIC_1_DATA_PORT::write(PIC_ICW2_OFFSET); /* ICW2: route IRQs 0...7 to INTs 20h...27h */
    PIC_1_DATA_PORT::write(0x04); /* ICW3 */
    PIC_1_DATA_PORT::write(PIC_ICW4_8086); /* PIC_ICW4 */

    PIC_2_CONTROL_PORT::write(PIC_ICW1_INIT);
    PIC_2_DATA_PORT::write(PIC_ICW2_OFFSET + 0x8); /* ...IRQs 8...15 to INTs 28h...2Fh */
    PIC_2_DATA_PORT::write(0x02);
    PIC_2_DATA_PORT::write(PIC_ICW4_8086);

    // Mask all interrupts
    setIrqMask(0xFFFF);

    for (size_t i=0; i < 16; ++i)
        sendEOI(i);

    outstanding_EOIs_ = 0;
}

bool PIC8259::isIRQEnabled(uint16 number)
{
  assert(number < 16);
  return !(cached_mask & (1 << number));
}

void PIC8259::enableIRQ(uint16 number)
{
  debug(PIC_8259, "PIC8259, enable IRQ %x\n", number);
  assert(number < 16);
  cached_mask &= ~(1 << number);
  if (number >= 8)
  {
    PIC_2_DATA_PORT::write(cached_mask >> 8);
  }
  else
  {
    PIC_1_DATA_PORT::write(cached_mask % 8);
  }

  if(!isIRQEnabled(2) && (number >= 8))
  {
    enableIRQ(2); // Enable slave cascade
  }
}

void PIC8259::disableIRQ(uint16 number)
{
  debug(PIC_8259, "PIC8259, disable IRQ %x\n", number);
  assert(number < 16);
  cached_mask |= (1 << number);
  if (number >= 8)
  {
    PIC_2_DATA_PORT::write(cached_mask >> 8);
  }
  else
  {
    PIC_1_DATA_PORT::write(cached_mask & 0xFF);
  }

  if(((cached_mask & 0xFF00) == 0xFF00) && isIRQEnabled(2) && (number >= 8))
  {
    disableIRQ(2); // Disable slave cascade
  }
}

void PIC8259::setIrqMask(uint16 mask)
{
  debug(PIC_8259, "PIC8259, set IRQ mask %x\n", mask);
  cached_mask = mask;
  PIC_2_DATA_PORT::write(cached_mask >> 8);
  PIC_1_DATA_PORT::write(cached_mask & 0xFF);
}

void PIC8259::sendEOI(uint16 number)
{
  debugAdvanced(PIC_8259, "PIC8259, send EOI for IRQ %x\n", number);

  assert(number <= 16);
  --outstanding_EOIs_;

  if (number >= 8)
  {
    PIC_2_CONTROL_PORT::write(PIC_EOI);
  }

  PIC_1_CONTROL_PORT::write(PIC_EOI);
}

PIC8259Driver::PIC8259Driver() :
    BasicDeviceDriver("PIC8259 driver")
{
}

PIC8259Driver& PIC8259Driver::instance()
{
    static PIC8259Driver i;
    return i;
}

void PIC8259Driver::doDeviceDetection()
{
    debug(DRIVER, "Driver '%s' device detection\n", driverName().c_str());

    if (PIC8259::exists)
    {
        PIC8259::instance().init();
        bindDevice(PIC8259::instance());
    }
}
