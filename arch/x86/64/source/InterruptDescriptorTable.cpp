#include "InterruptDescriptorTable.h"

#include "debug.h"

#define LO_WORD(x) (((uint32)(x)) & 0x0000FFFFULL)
#define HI_WORD(x) ((((uint32)(x)) >> 16) & 0x0000FFFFULL)
#define LO_DWORD(x) (((uint64)(x)) & 0x00000000FFFFFFFFULL)
#define HI_DWORD(x) ((((uint64)(x)) >> 32) & 0x00000000FFFFFFFFULL)

InterruptGateDesc::InterruptGateDesc(handler_func_t offset, uint8 dpl) :
    segment_selector(KERNEL_CS),
    ist(0),
    zeros(0),
    type(INTERRUPT),
    zero_1(0),
    dpl(dpl),
    present(1),
    reserved(0)
{
    setOffset(offset);
}

void InterruptGateDesc::setOffset(handler_func_t offset)
{
    offset_ld_lw = LO_WORD(LO_DWORD((uintptr_t)offset));
    offset_ld_hw = HI_WORD(LO_DWORD((uintptr_t)offset));
    offset_hd = HI_DWORD((uintptr_t)offset);
}

handler_func_t InterruptGateDesc::offset()
{
    return (handler_func_t)(((uintptr_t)offset_hd << 32) |
                            ((uintptr_t)offset_ld_hw << 16) | ((uintptr_t)offset_ld_lw));
}

void IDTR::load()
{
    debug(A_INTERRUPTS, "Loading IDT, base: %zx, limit: %x\n", base, limit);
    asm volatile("lidt (%0) " : : "q"(this));
}
