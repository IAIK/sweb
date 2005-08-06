//----------------------------------------------------------------------
//   $Id: ArchCommon.cpp,v 1.2 2005/08/06 17:38:35 rotho Exp $
//----------------------------------------------------------------------
//
//  $Log: ArchCommon.cpp,v $
//  Revision 1.1  2005/08/01 08:18:59  nightcreature
//  initial release, partly dummy implementation, needs changes
//
//
//----------------------------------------------------------------------


#include "ArchCommon.h"
//#include "multiboot.h"
//#include "boot-time.h"
#include "offsets.h"
#include "kprintf.h"

#define MAX_MEMORY_MAPS 10
#define FOUR_ZEROS 0,0,0,0
#define EIGHT_ZEROS FOUR_ZEROS,FOUR_ZEROS
#define SIXTEEN_ZEROS EIGHT_ZEROS,EIGHT_ZEROS
#define TWENTY_ZEROS SIXTEEN_ZEROS,FOUR_ZEROS
#define FOURTY_ZEROS TWENTY_ZEROS,TWENTY_ZEROS

struct memory_maps
{
  uint32 used;
  pointer start_address;
  pointer end_address;
  uint32 type;
} __attribute__((__packed__)) memory_maps;

static struct memory_maps mem_map_[MAX_MEMORY_MAPS] = {FOURTY_ZEROS};

// extern multiboot_info_t multi_boot_structure_pointer[];

  
extern "C" void parseMultibootHeader();

//we don't need this one..maybe remove it completly
void parseMultibootHeader()
{
  return;
}

//no we don't have one at all (at the moment)
uint32 ArchCommon::haveVESAConsole(uint32 is_paging_set_up)
{
  return 0;
}

//no we don't have vesa at all (at the moment)
uint32 ArchCommon::getVESAConsoleHeight()
{
  return 0;
}

//no we don't have vesa at all (at the moment)
uint32 ArchCommon::getVESAConsoleWidth()
{
  return 0;
}

//no we don't have vesa at all (at the moment)
pointer ArchCommon::getVESAConsoleLFBPtr(uint32 is_paging_set_up)
{
  return 0;
}

//TODO: check if it is still possible...or fake it somehow
//this will be done differently... make it 0 at the moment
pointer ArchCommon::getFBPtr(uint32 is_paging_set_up)
{
  return 0;
//   if (is_paging_set_up)
//     return 0xC00B8000;
//   else
//     return 0x000B8000;
}

//no we don't have vesa at all (at the moment)
uint32 ArchCommon::getVESAConsoleBitsPerPixel()
{
  return 0;
}


uint32 ArchCommon::getNumUseableMemoryRegions()
{
  uint32 i;
  for (i=0;i<MAX_MEMORY_MAPS;++i)
  {
    if (!mem_map_[i].used)
      break;
  }
  return i;
}


uint32 ArchCommon::getUsableMemoryRegion(uint32 region, pointer &start_address, pointer &end_address, uint32 &type)
{
  if (region >= MAX_MEMORY_MAPS)
    return 1;
  
  start_address = mem_map_[region].start_address;
  end_address = mem_map_[region].end_address;
  type = mem_map_[region].type;
  
  return 0;
}

#define MEMCOPY_LARGE_TYPE uint32

//maybe no changes needed so unchanged at the moment
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

//maybe no changes needed so unchanged at the moment
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
