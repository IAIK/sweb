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
   return (pointer)(1024U*1024U*1024U*2U + 1024U*1024U*4U); //2GB+4MB Ende des Kernel Bereichs fuer den es derzeit Paging gibt
}


uint32 ArchCommon::haveVESAConsole(uint32 is_paging_set_up)
{
  return false;
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
  return 0x80400000U;
}

uint32 ArchCommon::getVESAConsoleHeight()
{
  return 600;
}

uint32 ArchCommon::getVESAConsoleWidth()
{
  return 800;
}

pointer ArchCommon::getVESAConsoleLFBPtr(uint32 is_paging_set_up)
{
  return 0;
}

pointer ArchCommon::getFBPtr(uint32 is_paging_set_up)
{
  return 0;
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
  end_address = 8*1024*1024;
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


Console* ArchCommon::createConsole(uint32 count)
{
  if (haveVESAConsole())
    return new FrameBufferConsole(count);
  else
    return new TextConsole(count);
}

void ArchCommon::initDebug()
{
  extern unsigned char stab_start_address_nr;
  extern unsigned char stab_end_address_nr;

  extern unsigned char stabstr_start_address_nr;
  extern unsigned char stabstr_end_address_nr;

  parse_symtab((StabEntry*)&stab_start_address_nr, (StabEntry*)&stab_end_address_nr, (const char*)&stabstr_start_address_nr);
}

void ArchCommon::idle()
{
  //__asm__ __volatile__ ( "hlt" );
}
