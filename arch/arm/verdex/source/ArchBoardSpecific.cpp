/**
 * @file arch_board_specific.cpp
 *
 */

#include "ArchBoardSpecific.h"

#include "KeyboardManager.h"
#include "board_constants.h"
#include "InterruptUtils.h"
#include "ArchCommon.h"
#include "assert.h"
#include "offsets.h"
#include "ArchInterrupts.h"
#include "Scheduler.h"
#include "FrameBufferConsole.h"

#define PHYSICAL_MEMORY_AVAILABLE 8*1024*1024

pointer ArchBoardSpecific::getVESAConsoleLFBPtr()
{
  return 0xC0000000 | ((PHYSICAL_MEMORY_AVAILABLE - ArchCommon::getVESAConsoleWidth() * ArchCommon::getVESAConsoleHeight() * ArchCommon::getVESAConsoleBitsPerPixel() / 8) & ~0xFFF);
}

uint32 ArchBoardSpecific::getUsableMemoryRegion(uint32 region __attribute__((unused)), pointer &start_address, pointer &end_address, uint32 &type)
{
  start_address = 0;
  end_address = getVESAConsoleLFBPtr() & ~0xC0000000;
  type = 1;
  return 0;
}

uint32 frame_descriptor[4] __attribute__ ((aligned (0x10)));

void ArchBoardSpecific::frameBufferInit()
{
  uint32 num_bytes = ArchCommon::getVESAConsoleWidth() * ArchCommon::getVESAConsoleHeight() * ArchCommon::getVESAConsoleBitsPerPixel() / 8;
  frame_descriptor[3] = num_bytes << 2;
  frame_descriptor[2] = 0; // ID not used
  frame_descriptor[1] = (uint32)getVESAConsoleLFBPtr() - 0xC0000000 + 0xA0000000; // frame buffer address
  frame_descriptor[0] = (uint32)(frame_descriptor) - 0x80000000 + 0xA0000000; // address of the next frame descriptor (always repeat the same)
  uint32* DMA0 = (uint32*) 0x90000200;
  DMA0[0] = frame_descriptor[0];
  uint32* PRSR = (uint32*) 0x90000104;
  *PRSR = (0x3 << 9); // status OK & continue to next command
  uint32* LCCR = (uint32*) 0x90000000;
  LCCR[1] = 639;
  LCCR[2] = 479;
  LCCR[3] = (0x4 << 24); // 16 bit color
  LCCR[0] = 0x1; // enable lcd controller
}

void ArchBoardSpecific::onIdle()
{
}

void ArchBoardSpecific::enableTimer()
{
  uint32* picmmio = (uint32*)0x84000004;
  *picmmio |= (1<<26);

  uint32* osmr0 = (uint32*)0x83000000;
  *osmr0 = 200000;
  uint32* oeir = (uint32*)0x8300001C;
  *oeir |= 1;
  uint32* oscr = (uint32*)0x83000010;
  *oscr = 0;
}

void ArchBoardSpecific::disableTimer()
{
  uint32* picmmio = (uint32*)0x84000004;
  *picmmio &= ~(1<<26);

  uint32* oeir = (uint32*)0x8300001C;
  *oeir &= ~1;
}


void ArchBoardSpecific::enableKBD()
{
  uint32* picmmio = (uint32*)0x84000004;
  *picmmio |= (1<<22);

  *(uint32*)(SERIAL_BASE + 0x8) = 0x7;
  *(uint32*)(SERIAL_BASE + 0x4) = 0x1;
}


void ArchBoardSpecific::disableKBD()
{
}

void ArchBoardSpecific::keyboard_irq_handler()
{
  KeyboardManager::instance()->serviceIRQ();
}

extern void timer_irq_handler();

void ArchBoardSpecific::timer0_irq_handler()
{
  uint32 *ossr = (uint32*)0x83000014;
  if ((*ossr & 0x1) != 0)
  {
    assert(!ArchInterrupts::testIFSet());
    *ossr = 1;
    uint32* oscr = (uint32*)0x83000010;
    *oscr = 0;

    timer_irq_handler();
  }
}

void ArchBoardSpecific::uart0_irq_handler()
{
  KeyboardManager::instance()->serviceIRQ();
  *(uint32*)(SERIAL_BASE + 0x8) = 0x6;
}

extern void arch_swi_irq_handler();

#define IRQ(X) ((*pic) & (1 << X))
void ArchBoardSpecific::irq_handler()
{
  uint32* pic = (uint32*)PIC_BASE;
  if (IRQ(0))
    arch_swi_irq_handler();
  if (IRQ(22))
    uart0_irq_handler();
  if (IRQ(26))
    timer0_irq_handler();
  // 6-10 and 22-28 not implemented
}



