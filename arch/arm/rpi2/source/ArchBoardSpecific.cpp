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
#include "kprintf.h"

#define PHYSICAL_MEMORY_AVAILABLE 8*1024*1024

struct RPiFrameBufferStructure
{
  uint32 width;
  uint32 height;
  uint32 vwidth;
  uint32 vheight;
  uint32 pitch;
  uint32 depth;
  uint32 xoffset;
  uint32 yoffset;
  uint32 pointer;
  uint32 size;
  uint16 cmap[256];
};
struct RPiFrameBufferStructure fbs __attribute__ ((aligned (0x20)));
pointer framebuffer;

pointer ArchBoardSpecific::getVESAConsoleLFBPtr()
{
  return framebuffer;
}

size_t ArchBoardSpecific::getUsableMemoryRegion(size_t region __attribute__((unused)), pointer &start_address, pointer &end_address, size_t &type)
{
  start_address = 0;
  end_address = ((PHYSICAL_MEMORY_AVAILABLE - ArchCommon::getVESAConsoleWidth() * ArchCommon::getVESAConsoleHeight() * ArchCommon::getVESAConsoleBitsPerPixel() / 8) & ~0xFFF);
  type = 1;
  return 0;
}

extern "C" void memory_barrier();

void ArchBoardSpecific::frameBufferInit()
{
  // frame buffer initialization from http://elinux.org/RPi_Framebuffer#Notes
  for (uint32 i = 0; i < 10 && (fbs.pointer == 0 || fbs.size == 0); ++i)
  {
    fbs.width = 640;
    fbs.height = 480;
    fbs.vwidth = fbs.width;
    fbs.vheight = fbs.height;
    fbs.pitch = 0;
    fbs.depth = 16;
    fbs.xoffset = 0;
    fbs.yoffset = 0;
    fbs.pointer = 0;
    fbs.size = 0;
    uint32* MAIL0_READ = (uint32*)0x9000b880;
    uint32* MAIL0_WRITE = (uint32*)0x9000b8A0;
    uint32* MAIL0_STATUS = (uint32*)0x9000b898;
    memory_barrier();
    while (*MAIL0_STATUS & (1 << 31));
    assert((((uint32)&fbs) & 0xF) == 0);
    *MAIL0_WRITE = VIRTUAL_TO_PHYSICAL_BOOT(((uint32)&fbs) & ~0xF) | (0x1);
    memory_barrier();
    uint32 read = 0;
    while ((read & 0xF) != 1)
    {
      while (*MAIL0_STATUS & (1 << 30));
      read = *MAIL0_READ;
    }
    memory_barrier();
    for (uint32 i = 0; i < 0x10000; ++i);
  }
  assert(fbs.pointer != 0);
  assert(fbs.width == fbs.vwidth);
  assert(fbs.height == fbs.vheight);
  assert(fbs.size == (fbs.width * fbs.height * fbs.depth / 8));
  framebuffer = (fbs.pointer & ~0xC0000000) + 0xC0000000;
}

void ArchBoardSpecific::onIdle()
{
  keyboard_irq_handler(); // TODO: this is not only ugly polling, but we're losing keys all the time
  // and this is in an ugly place for keyboard handling! why is it here? we would have needed a whole new thread otherwise
  // the usb stack should work with less dynamic memory and more stack variables, then it would be less complicated
}

#define USE_SYSTEM_TIMER

size_t timer_loading_value = 0;
void ArchBoardSpecific::enableTimer()
{
#ifdef USE_SYSTEM_TIMER
    debug(A_BOOT, "Enabling system timer, addr %p\n", (uint32*)ARM_CORE0_TIM_IRQCNTL);
  size_t timer_prescaler = 20;     // (1/prescaler) s = prescaler Hz
  uint32 timer_frequency = 0;
  asm("mrc p15, 0, %0, c14, c0, 0\n"
      : "=r" (timer_frequency));

  timer_loading_value = timer_frequency / timer_prescaler;
  debug(A_BOOT, "Setting timer tval: %u\n", timer_loading_value);
  asm("mcr p15, 0, %0, c14, c3, 0\n"
      :: "r" (timer_loading_value));

  asm("mrc p15, 0, %0, c14, c3, 0\n"
      : "=r" (timer_loading_value));
  debug(A_BOOT, "Remaining timer tval: %u\n", timer_loading_value);

  // CNTV_CTL
  // enable timer
  asm("mcr p15, 0, %0, c14, c3, 1\n"
      :: "r" (1));

  debug(A_BOOT, "Timer enabled, freq: %u\n", timer_frequency);


  asm("mrc p15, 0, %0, c14, c3, 0\n"
      : "=r" (timer_loading_value));
  debug(A_BOOT, "Remaining timer tval: %u\n", timer_loading_value);

  asm("mrc p15, 0, %0, c14, c3, 0\n"
      : "=r" (timer_loading_value));
  debug(A_BOOT, "Remaining timer tval: %u\n", timer_loading_value);

  uint32* core0_irq_control = (uint32*)(ARM_CORE0_TIM_IRQCNTL);
  *core0_irq_control = TIM_IRQCNTL_CNTVIRQ_IRQ;
#else
  // This timer is not actually implemented in QEMU hardware emulation

  // uint32* pic_base_enable = (uint32*)0x9000B218;
  // *pic_base_enable = 0x1;
  *PIC_ENABLE_BASIC = 1;

  // uint32* timer_load = (uint32*)0x9000B400;
  // //uint32* timer_value = timer_load + 1;
  // *timer_load = 0x800;
  *TIMER_LOAD = 0x800;
  // uint32* timer_control = timer_load + 2;
  // *timer_control = (1 << 7) | (1 << 5) | (1 << 2);
  *TIMER_CTRL = TIMER_IRQ_ENABLE | TIMER_CTRL_ENABLE | (1 << 2);
  // uint32* timer_clear = timer_load + 3;
  // *timer_clear = 0x1;
  *TIMER_CLEAR = 1;
#endif
}

void ArchBoardSpecific::setTimerFrequency([[maybe_unused]]uint32 freq)
{
  debug(A_BOOT, "Sorry, setTimerFrequency not implemented!\n");
}

void ArchBoardSpecific::disableTimer()
{
}

void ArchBoardSpecific::enableKBD()
{

}

void ArchBoardSpecific::disableKBD()
{
}

void ArchBoardSpecific::keyboard_irq_handler()
{
  KeyboardManager::instance()->serviceIRQ();
}

void resetTimer()
{
    asm("mcr p15, 0, %0, c14, c3, 0\n" :: "r" (timer_loading_value));
}

extern void timer_irq_handler();

void ArchBoardSpecific::timer0_irq_handler()
{
#ifdef USE_SYSTEM_TIMER
    resetTimer();
    timer_irq_handler();
#else
  uint32* timer_raw = (uint32*)0x9000B410;
  if ((*timer_raw & 0x1) != 0)
  {
    assert(!ArchInterrupts::testIFSet());
    uint32* timer_clear = (uint32*)0x9000B40C;
    *timer_clear = 0x1;

    timer_irq_handler();
  }
#endif
}

#define IRQ(X) ((*pic) & (1 << X))
void ArchBoardSpecific::irq_handler()
{
#ifdef USE_SYSTEM_TIMER
  uint32* core0_irq_data = (uint32*)ARM_CORE0_IRQ_SOURCE;
  if (*core0_irq_data & (1 << 3))
  {
      timer0_irq_handler();
  }
#else

  uint32* pic = (uint32*)PIC_BASE;
  if (IRQ(0))
  {
    timer0_irq_handler();
  }
#endif
}

extern "C" void ArchBoardSpecific::disableMulticore(uint32* spin) {
  asm("mov r1, #0x1\n"
          "multicore_spin:\n"
          "ldrex r0, [%[r]]\n" // load the sync value
          "cmp r0, #0\n" // check whether this is the first core
          "strexeq r0, r1, [%[r]]\n"  // if yes, claim the sync value
          "cmpeq r0, #0\n" // did this succeed?
          "bne multicore_spin\n" // no - we are not the first core, spin indefinitely
  : : [r]"r"(spin));
}
