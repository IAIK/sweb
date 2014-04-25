/**
 * @file ArchCommon.cpp
 *
 */

#include "ArchCommon.h"
#include "boot-time.h"
#include "offsets.h"
#include "kprintf.h"
#include "ArchMemory.h"
#include "TextConsole.h"
#include "FrameBufferConsole.h"
#include "backtrace.h"

#define PHYSICAL_MEMORY_AVAILABLE 8*1024*1024

extern void* kernel_end_address;

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
struct RPiFrameBufferStructure fbs;
pointer framebuffer;

pointer ArchCommon::getKernelEndAddress()
{
   return (pointer)&kernel_end_address;
}

pointer ArchCommon::getFreeKernelMemoryStart()
{
   return (pointer)&kernel_end_address;
}

pointer ArchCommon::getFreeKernelMemoryEnd()
{
   return (pointer)getModuleEndAddress(0);
}


uint32 ArchCommon::haveVESAConsole(uint32 is_paging_set_up)
{
  return true;
}

uint32 ArchCommon::getNumModules(uint32 is_paging_set_up)
{
  return 1;
}

uint32 ArchCommon::getModuleStartAddress(uint32 num, uint32 is_paging_set_up)
{
  return 0x80000000U;
}

uint32 ArchCommon::getModuleEndAddress(uint32 num, uint32 is_paging_set_up)
{
  return 0x80400000U;  //2GB+4MB Ende des Kernel Bereichs
}

uint32 ArchCommon::getVESAConsoleHeight()
{
  return 480;
}

uint32 ArchCommon::getVESAConsoleWidth()
{
  return 640;
}

pointer ArchCommon::getVESAConsoleLFBPtr(uint32 is_paging_set_up)
{
  return framebuffer;
}

pointer ArchCommon::getFBPtr(uint32 is_paging_set_up)
{
  return getVESAConsoleLFBPtr();
}

uint32 ArchCommon::getVESAConsoleBitsPerPixel()
{
  return 16;
}

uint32 ArchCommon::getNumUseableMemoryRegions()
{
  return 1;
}

uint32 ArchCommon::getUsableMemoryRegion(uint32 region, pointer &start_address, pointer &end_address, uint32 &type)
{
  start_address = 0;
  end_address = ((PHYSICAL_MEMORY_AVAILABLE - getVESAConsoleWidth() * getVESAConsoleHeight() * getVESAConsoleBitsPerPixel() / 8) & ~0xFFF);
  type = 1;
  return 0;
}

#define MEMCOPY_LARGE_TYPE uint32

void ArchCommon::memcpy(pointer dest, pointer src, size_t size)
{
  MEMCOPY_LARGE_TYPE *s64 = (MEMCOPY_LARGE_TYPE*)src;
  MEMCOPY_LARGE_TYPE *d64 = (MEMCOPY_LARGE_TYPE*)dest;

  uint32 i;
  uint32 num_64_bit_copies = size / (sizeof(MEMCOPY_LARGE_TYPE)*8);
  uint32 num_8_bit_copies = size % (sizeof(MEMCOPY_LARGE_TYPE)*8);

  for (i=0;i<num_64_bit_copies;++i)
  {
    d64[0] = s64[0];
    d64[1] = s64[1];
    d64[2] = s64[2];
    d64[3] = s64[3];
    d64[4] = s64[4];
    d64[5] = s64[5];
    d64[6] = s64[6];
    d64[7] = s64[7];
    d64 += 8;
    s64 += 8;
  }

  uint8 *s8 = (uint8*)s64;
  uint8 *d8 = (uint8*)d64;

  for (i=0;i<num_8_bit_copies;++i)
  {
    *d8 = *s8;
    ++d8;
    ++s8;
  }
}

void ArchCommon::bzero(pointer s, size_t n, uint32 debug)
{
  if (debug) kprintf("Bzero start\n");
  MEMCOPY_LARGE_TYPE *s64 = (MEMCOPY_LARGE_TYPE*)s;
  uint32 num_64_bit_zeros = n / sizeof(MEMCOPY_LARGE_TYPE);
  uint32 num_8_bit_zeros = n % sizeof(MEMCOPY_LARGE_TYPE);
  uint32 i;
  if (debug) kprintf("Bzero next\n");
  for (i=0;i<num_64_bit_zeros;++i)
  {
    *s64 = 0;
    ++s64;
  }
  uint8 *s8 = (uint8*)s64;
  if (debug) kprintf("Bzero middle\n");
  for (i=0;i<num_8_bit_zeros;++i)
  {
    *s8 = 0;
    ++s8;
  }
  if (debug) kprintf("Bzero end\n");
}

uint32 ArchCommon::checksumPage(uint32 physical_page_number, uint32 page_size)
{
  uint32 *src = (uint32*)ArchMemory::getIdentAddressOfPPN(physical_page_number);

  uint32 poly = 0xEDB88320;
  int bit = 0, nbits = 32;
  uint32 res = 0xFFFFFFFF;

  for (uint32 i = 0; i < page_size/sizeof(*src); ++i)
    for (bit = nbits - 1; bit >= 0; --bit)
      if ((res & 1) != ((src[i] >> bit) & 1))
        res = (res >> 1) ^ poly;
      else
        res = (res >> 1) + 7;

  res ^= 0xFFFFFFFF;
  return res;
}

extern "C" void memory_barrier();

Console* ArchCommon::createConsole(uint32 count)
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
    while (read & 0xF != 1)
    {
      while (*MAIL0_STATUS & (1 << 30));
      read = *MAIL0_READ;
    }
    memory_barrier();
    for (uint32 i = 0; i < 0x10000; ++i);
//    kprintfd("fbs.width: %x\n",fbs.width);
//    kprintfd("fbs.height: %x\n",fbs.height);
//    kprintfd("fbs.vwidth: %x\n",fbs.vwidth);
//    kprintfd("fbs.vheight: %x\n",fbs.vheight);
//    kprintfd("fbs.pitch: %x\n",fbs.pitch);
//    kprintfd("fbs.depth: %x\n",fbs.depth);
//    kprintfd("fbs.xoffset: %x\n",fbs.xoffset);
//    kprintfd("fbs.yoffset: %x\n",fbs.yoffset);
//    kprintfd("fbs.pointer: %x\n",fbs.pointer);
//    kprintfd("fbs.size: %x\n",fbs.size);
  }
  assert(fbs.pointer != 0);
  assert(fbs.width == fbs.vwidth);
  assert(fbs.height == fbs.vheight);
  //assert(fbs.pitch == (fbs.width * fbs.depth / 8));
  assert(fbs.size == (fbs.width * fbs.height * fbs.depth / 8));
  framebuffer = (fbs.pointer & ~0xC0000000) + 0xC0000000;
//  if (fbs.pointer + fbs.size < 0x40000000)
//    framebuffer = fbs.pointer + 0xC0000000;
//  else if ((fbs.pointer > 0x5c000000) && (fbs.pointer + fbs.size < 0x5c800000))
//    framebuffer = fbs.pointer + 0xB0000000 - 0x5c000000;
//  else
//    assert(false);
//  kprintfd("returning with framebuffer: %x and size: %x\n", fbs.pointer, fbs.size);
//  kprintfd("returning with framebuffer: %x and size: %x\n", framebuffer, fbs.size);
  return new FrameBufferConsole(count);
}

void ArchCommon::initDebug()
{
  extern unsigned char stab_start_address_nr;
  extern unsigned char stab_end_address_nr;

  extern unsigned char stabstr_start_address_nr;

  parse_symtab((StabEntry*)&stab_start_address_nr, (StabEntry*)&stab_end_address_nr, (const char*)&stabstr_start_address_nr);
}

extern "C" void halt();

void ArchCommon::idle()
{
  halt();
}
