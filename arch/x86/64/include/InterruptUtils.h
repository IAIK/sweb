#pragma once

#include "types.h"

typedef void(*handler_func_t)(void);

typedef struct {
    uint32             number;  // handler number
    handler_func_t handler_func; // pointer to handler function
}  __attribute__((__packed__)) InterruptHandlers;


typedef struct IDTR {
    uint16 limit;
    size_t base;

    void load();
} __attribute__((__packed__)) IDTR;


struct InterruptGateDesc
{
    uint16_t offset_ld_lw     : 16; // low word / low dword of handler entry point's address
    uint16_t segment_selector : 16; // (code) segment the handler resides in
    uint8_t  ist              :  3; // interrupt stack table index
    uint8_t  zeros            :  5; // set to zero
    uint8_t  type             :  4; // set to TYPE_TRAP_GATE or TYPE_INTERRUPT_GATE
    uint8_t  zero_1           :  1; // unsued - set to zero
    uint8_t  dpl              :  2; // descriptor protection level
    uint8_t  present          :  1; // present- flag - set to 1
    uint16_t offset_ld_hw     : 16; // high word / low dword of handler entry point's address
    uint32_t offset_hd        : 32; // high dword of handler entry point's address
    uint32_t reserved         : 32;

    InterruptGateDesc() = default;
    InterruptGateDesc(handler_func_t offset, uint8_t dpl);

    void setOffset(handler_func_t);
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
}__attribute__((__packed__));


class InterruptUtils
{
public:
    static void initialise();

    static IDTR idtr;

    static InterruptGateDesc *idt;

private:
    static InterruptHandlers handlers[];
};
