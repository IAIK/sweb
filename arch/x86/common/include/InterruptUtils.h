#pragma once

#include "types.h"

typedef struct {
  uint32  number;      // handler number
  void (*offset)();    // pointer to handler function
}  __attribute__((__packed__)) InterruptHandlers;


typedef struct {
    uint16 limit;
    size_t base;

    void load();
} __attribute__((__packed__)) IDTR ;

struct InterruptGateDesc
{
        uint16 offset_ld_lw : 16;     // low word / low dword of handler entry point's address
        uint16 segment_selector : 16; // (code) segment the handler resides in
        uint8 ist       : 3;     // interrupt stack table index
        uint8 zeros     : 5;     // set to zero
        uint8 type      : 4;     // set to TYPE_TRAP_GATE or TYPE_INTERRUPT_GATE
        uint8 zero_1    : 1;     // unsued - set to zero
        uint8 dpl       : 2;     // descriptor protection level
        uint8 present   : 1;     // present- flag - set to 1
        uint16 offset_ld_hw : 16;     // high word / low dword of handler entry point's address
        uint32 offset_hd : 32;        // high dword of handler entry point's address
        uint32 reserved : 32;

        InterruptGateDesc() = default;
        InterruptGateDesc(uint64 offset, uint8 dpl);

        void setOffset(uint64 offset);
}__attribute__((__packed__));


class InterruptUtils
{
public:
  static void initialise();

  static void countPageFault(uint64 address);

  static IDTR idtr;

  static InterruptGateDesc *idt;

private:
  static InterruptHandlers handlers[];
  static uint64 pf_address;
  static uint64 pf_address_counter;
};

