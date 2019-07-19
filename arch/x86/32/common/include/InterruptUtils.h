#pragma once

#include "types.h"

#define DPL_KERNEL_SPACE     0 // kernelspace's protection level
#define DPL_USER_SPACE       3 // userspaces's protection level

#define SYSCALL_INTERRUPT 0x80 // number of syscall interrupt

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
        uint16 offset_low       : 16;     // low word of handler entry point's address
        uint16 segment_selector : 16; // (code) segment the handler resides in
        uint8 unused            : 8;     // set to zero
        uint8 type              : 4;     // set to TYPE_TRAP_GATE or TYPE_INTERRUPT_GATE
        uint8 zero_1            : 1;     // unused - set to zero
        uint8 dpl               : 2;     // descriptor protection level
        uint8 present           : 1;     // present- flag - set to 1
        uint16 offset_high      : 16;     // high word of handler entry point's address

        InterruptGateDesc() = default;
        InterruptGateDesc(size_t offset, uint8 dpl);

        void setOffset(size_t offset);
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
