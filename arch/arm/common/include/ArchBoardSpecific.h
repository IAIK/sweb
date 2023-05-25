#pragma once

#include "types.h"

class ArchBoardSpecific
{
  public:
    static pointer getVESAConsoleLFBPtr();
    static size_t getUsableMemoryRegion(size_t region, pointer &start_address, pointer &end_address, size_t &type);
    static void frameBufferInit();
    static void onIdle();
    static void enableTimer();
    static void setTimerFrequency(uint32 freq);
    static void disableTimer();
    static void enableKBD();
    static void disableKBD();
    static void keyboard_irq_handler();
    static void timer0_irq_handler();
    static void irq_handler();
    static void uart0_irq_handler();
    static void disableMulticore(uint32* spin);
};
