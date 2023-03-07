#pragma once

#include <cstddef>
#include <cstdint>
#include "SegmentUtils.h"
#include "EASTL/array.h"

using handler_func_t = void (*)();

static constexpr size_t NUM_INTERRUPTS = 256;

// https://wiki.osdev.org/Exceptions
constexpr bool interruptHasErrorcode(size_t N)
{
    if ((N == 8) || (10 <= N && N <= 14) || (N == 17) || (N == 21) || (N == 29) ||
        (N == 30))
        return true;

    return false;
}

constexpr int interruptPrivilegeLevel(size_t interrupt_number)
{
    if (interrupt_number == 0x80)
        return DPL_USER;
    else
        return DPL_KERNEL;
}

struct [[gnu::packed]] InterruptGateDesc
{
    uint16_t offset_low       : 16; // low word of handler entry point's address
    uint16_t segment_selector : 16; // (code) segment the handler resides in
    uint8_t unused            : 8;  // set to zero
    uint8_t type              : 4;  // set to TYPE_TRAP_GATE or TYPE_INTERRUPT_GATE
    uint8_t zero_1            : 1;  // unused - set to zero
    uint8_t dpl               : 2;  // descriptor protection level
    uint8_t present           : 1;  // present- flag - set to 1
    uint16_t offset_high      : 16; // high word of handler entry point's address

    InterruptGateDesc() = default;
    InterruptGateDesc(handler_func_t offset, uint8_t dpl);

    void setOffset(handler_func_t offset);
    handler_func_t offset();

    enum Type
    {
        // interrupt gate => IF flag *is* cleared
        // trap gate      => IF flag is *not* cleared
        TASK            = 0x5,
        INTERRUPT_16BIT = 0x6,
        TRAP_16BIT      = 0x7,
        INTERRUPT       = 0xE,
        TRAP            = 0xF,
    };
};

struct [[gnu::packed]] IDTR
{
    uint16_t limit;
    uintptr_t base;

    void load();
};

/** Interrupt entry stub.
 *  Pushes interrupt number onto the stack before jumping to the main
 *  interrupt hander arch_interruptHandler(). Also pushes a fake error code if
 *  not already done by the processor itself to ensure stack layout consistency.
 *
 *  @tparam N interrupt number
 */
template<size_t N> [[gnu::naked, noreturn]] void interruptEntry()
{
    // compile time constexpr if -> push instruction is only generated if required. No check at runtime
    if constexpr (!interruptHasErrorcode(N))
        asm volatile("pushl $0\n");

    // Main interrupt handler code is responsible for saving registers, etc...
    asm volatile("pushl %[num]\n"
                 "jmp arch_interruptHandler\n" ::[num] "i"(N));
}

template<size_t I> auto make_element()
{
    return InterruptGateDesc{&interruptEntry<I>, interruptPrivilegeLevel(I)};
}

template<typename T, size_t... NN>
constexpr auto generate_impl(eastl::index_sequence<NN...>)
    -> eastl::array<T, sizeof...(NN)>
{
    return {make_element<NN>()...};
}

template<typename T, size_t N> constexpr eastl::array<T, N> generate()
{
    return generate_impl<T>(eastl::make_index_sequence<N>());
}

struct InterruptDescriptorTable
{
    constexpr InterruptDescriptorTable() :
        entries(generate<InterruptGateDesc, NUM_INTERRUPTS>())
    {
    }

    constexpr IDTR idtr() { return {sizeof(entries) - 1, (uintptr_t)&entries}; }

    eastl::array<InterruptGateDesc, NUM_INTERRUPTS> entries;

    static_assert(sizeof(entries) == sizeof(InterruptGateDesc) * NUM_INTERRUPTS);
};
