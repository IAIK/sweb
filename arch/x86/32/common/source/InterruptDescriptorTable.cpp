#include "InterruptDescriptorTable.h"

#include "debug.h"

#define LO_WORD(x) (((uint32)(x)) & 0x0000FFFF)
#define HI_WORD(x) ((((uint32)(x)) >> 16) & 0x0000FFFF)

InterruptGateDesc::InterruptGateDesc(handler_func_t offset, uint8 dpl) :
    segment_selector(KERNEL_CS),
    unused(0),
    type(INTERRUPT),
    zero_1(0),
    dpl(dpl),
    present(1)
{
    setOffset(offset);
}

void InterruptGateDesc::setOffset(handler_func_t offset)
{
    offset_low = LO_WORD(offset);
    offset_high = HI_WORD(offset);
}

handler_func_t InterruptGateDesc::offset()
{
    return (handler_func_t)(((uintptr_t)offset_high << 16) | ((uintptr_t)offset_low));
}

void IDTR::load()
{
    debug(A_INTERRUPTS, "Loading IDT, base: %zx, limit: %x\n", base, limit);
    asm volatile("lidt (%0) " : : "q"(this));
}
