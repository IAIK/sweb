/**
 * @file arch_board_specific.h
 *
 */

#ifndef _BOARD_SPECIFIC_H_
#define _BOARD_SPECIFIC_H_

#include "types.h"

class ArchBoardSpecific
{
  public:
    static pointer getVESAConsoleLFBPtr();
    static uint32 getUsableMemoryRegion(uint32 region, pointer &start_address, pointer &end_address, uint32 &type);
    static void frameBufferInit();
    static void onIdle();
    static void enableTimer();
    static void disableTimer();
    static void enableKBD();
    static void disableKBD();
    static void keyboard_irq_handler();
    static void timer0_irq_handler();
    static void irq_handler();
    static void uart0_irq_handler();
};

#endif
