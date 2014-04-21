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
  return 0xC0000000 | ((PHYSICAL_MEMORY_AVAILABLE - getVESAConsoleWidth() * getVESAConsoleHeight() * getVESAConsoleBitsPerPixel() / 8) & ~0xFFF);
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
  end_address = getVESAConsoleLFBPtr(0);
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

uint32 frame_descriptor[4];

Console* ArchCommon::createConsole(uint32 count)
{
  uint32 num_bytes = getVESAConsoleWidth() * getVESAConsoleHeight() * getVESAConsoleBitsPerPixel() / 8;
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
  return new FrameBufferConsole(count);
}

void ArchCommon::initDebug()
{
  extern unsigned char stab_start_address_nr;
  extern unsigned char stab_end_address_nr;

  extern unsigned char stabstr_start_address_nr;
  extern unsigned char stabstr_end_address_nr;

  parse_symtab((StabEntry*)&stab_start_address_nr, (StabEntry*)&stab_end_address_nr, (const char*)&stabstr_start_address_nr);
}

extern "C" void halt();

void ArchCommon::idle()
{
  halt();
}
