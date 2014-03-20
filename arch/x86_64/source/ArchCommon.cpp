/**
 * @file ArchCommon.cpp
 *
 */

#include "ArchCommon.h"
#include "multiboot.h"
#include "debug_bochs.h"
#include "boot-time.h"
#include "offsets.h"
#include "kprintf.h"
#include "string.h"
#include "ArchMemory.h"
#include "FrameBufferConsole.h"
#include "TextConsole.h"

#define MAX_MEMORY_MAPS 10
#define MAX_MODULE_MAPS 10

// macros for initializing memory_maps
#define MEMMAP_INIT {0,0,0,0}
#define TWO_MEMMAP_INIT MEMMAP_INIT,MEMMAP_INIT
#define FOUR_MEMMAP_INIT TWO_MEMMAP_INIT,TWO_MEMMAP_INIT
#define FIVE_MEMMAP_INIT FOUR_MEMMAP_INIT,MEMMAP_INIT
#define TEN_MEMMAP_INIT FIVE_MEMMAP_INIT,FIVE_MEMMAP_INIT

// macros for initializing module_maps
#define FOUR_ZEROS 0,0,0,0
#define EIGHT_ZEROS FOUR_ZEROS,FOUR_ZEROS
#define SIXTEEN_ZEROS EIGHT_ZEROS,EIGHT_ZEROS
#define THIRTYTWO_ZEROS SIXTEEN_ZEROS,SIXTEEN_ZEROS
#define SIXTYFOUR_ZEROS THIRTYTWO_ZEROS,THIRTYTWO_ZEROS
#define HUNDREDTWENTYEIGHT_ZEROS SIXTYFOUR_ZEROS,SIXTYFOUR_ZEROS
#define TWOHUNDREDFIFTYSIX_ZEROS HUNDREDTWENTYEIGHT_ZEROS,\
                                 HUNDREDTWENTYEIGHT_ZEROS

#define MODMAP_INIT {0,0,0,{TWOHUNDREDFIFTYSIX_ZEROS}}
#define TWO_MODMAP_INIT MODMAP_INIT,MODMAP_INIT
#define FOUR_MODMAP_INIT TWO_MODMAP_INIT,TWO_MODMAP_INIT
#define FIVE_MODMAP_INIT FOUR_MODMAP_INIT,MODMAP_INIT
#define TEN_MODMAP_INIT FIVE_MODMAP_INIT,FIVE_MODMAP_INIT

extern void* kernel_end_address;

extern multiboot_info_t* multi_boot_structure_pointer;

struct multiboot_remainder mbr = {0,0,0,0,0,0,0,{TEN_MEMMAP_INIT},{TEN_MODMAP_INIT}};

extern "C" void parseMultibootHeader();

void parseMultibootHeader()
{
  uint32 i;
  multiboot_info_t *mb_infos = *(multiboot_info_t**)VIRTUAL_TO_PHYSICAL_BOOT( (pointer)&multi_boot_structure_pointer);
  struct multiboot_remainder &orig_mbr = (struct multiboot_remainder &)(*((struct multiboot_remainder*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&mbr)));

  if (mb_infos && mb_infos->flags & 1<<11)
  {
    struct vbe_mode* mode_info = (struct vbe_mode*)(uint64)mb_infos->vbe_mode_info;
    orig_mbr.have_vesa_console = 1;
    orig_mbr.vesa_lfb_pointer = mode_info->phys_base;
    orig_mbr.vesa_x_res = mode_info->x_resolution;
    orig_mbr.vesa_y_res = mode_info->y_resolution;
    orig_mbr.vesa_bits_per_pixel = mode_info->bits_per_pixel;
  }

  if (mb_infos && mb_infos->flags && 1<<3)
  {
    module_t * mods = (module_t*)(uint64)mb_infos->mods_addr;
    for (i=0;i<mb_infos->mods_count;++i)
    {
      orig_mbr.module_maps[i].used = 1;
      orig_mbr.module_maps[i].start_address = mods[i].mod_start;
      orig_mbr.module_maps[i].end_address = mods[i].mod_end;
      strncpy((char*)(uint64)orig_mbr.module_maps[i].name, (const char*)(uint64)mods[i].string, 256);
    }
    orig_mbr.num_module_maps = mb_infos->mods_count;
  }

  for (i=0;i<MAX_MEMORY_MAPS;++i)
  {
    orig_mbr.memory_maps[i].used = 0;
  }

  if (mb_infos && mb_infos->flags & 1<<6)
  {
    uint32 mmap_size = sizeof(memory_map);
    uint32 mmap_total_size = mb_infos->mmap_length;
    uint32 num_maps = mmap_total_size / mmap_size;

    for (i=0;i<num_maps;++i)
    {
      memory_map * map = (memory_map*)(uint64)(mb_infos->mmap_addr+mmap_size*i);
      orig_mbr.memory_maps[i].used = 1;
      orig_mbr.memory_maps[i].start_address = map->base_addr_low;
      orig_mbr.memory_maps[i].end_address = map->base_addr_low + map->length_low;
      orig_mbr.memory_maps[i].type = map->type;
    }
  }
}

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
   return 0xFFFFFFFF80400000ULL;
}


uint32 ArchCommon::haveVESAConsole(uint32 is_paging_set_up)
{
  if (is_paging_set_up)
    return mbr.have_vesa_console;
  else
  {
    struct multiboot_remainder &orig_mbr = (struct multiboot_remainder &)(*((struct multiboot_remainder*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&mbr)));
    return orig_mbr.have_vesa_console;
  }
}

uint32 ArchCommon::getNumModules(uint32 is_paging_set_up)
{
  if (is_paging_set_up)
    return mbr.num_module_maps;
  else
  {
    struct multiboot_remainder &orig_mbr = (struct multiboot_remainder &)(*((struct multiboot_remainder*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&mbr)));
    return orig_mbr.num_module_maps;
  }
}

uint32 ArchCommon::getModuleStartAddress(uint32 num,uint32 is_paging_set_up)
{
  if (is_paging_set_up)
    return mbr.module_maps[num].start_address | PHYSICAL_TO_VIRTUAL_OFFSET;
  else
  {
    struct multiboot_remainder &orig_mbr = (struct multiboot_remainder &)(*((struct multiboot_remainder*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&mbr)));
    return orig_mbr.module_maps[num].start_address | PHYSICAL_TO_VIRTUAL_OFFSET;
  }
}

uint32 ArchCommon::getModuleEndAddress(uint32 num,uint32 is_paging_set_up)
{
  if (is_paging_set_up)
    return mbr.module_maps[num].end_address | PHYSICAL_TO_VIRTUAL_OFFSET;
  else
  {
    struct multiboot_remainder &orig_mbr = (struct multiboot_remainder &)(*((struct multiboot_remainder*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&mbr)));
    return orig_mbr.module_maps[num].end_address | PHYSICAL_TO_VIRTUAL_OFFSET;
  }
}

uint32 ArchCommon::getVESAConsoleHeight()
{
  return mbr.vesa_y_res;
}

uint32 ArchCommon::getVESAConsoleWidth()
{
  return mbr.vesa_x_res;
}

pointer ArchCommon::getVESAConsoleLFBPtr(uint32 is_paging_set_up)
{
  if (is_paging_set_up)
    return 0xFFFFFFFFC000000ULL - 1024U * 1024U * 16U;
  else
  {
    struct multiboot_remainder &orig_mbr = (struct multiboot_remainder &)(*((struct multiboot_remainder*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&mbr)));
    return orig_mbr.vesa_lfb_pointer;
  }
}

pointer ArchCommon::getFBPtr(uint32 is_paging_set_up)
{
  if (is_paging_set_up)
    return PHYSICAL_TO_VIRTUAL_OFFSET | 0xB8000ULL;
  else
    return 0xB8000ULL;
}

uint32 ArchCommon::getVESAConsoleBitsPerPixel()
{
  return mbr.vesa_bits_per_pixel;
}

uint32 ArchCommon::getNumUseableMemoryRegions()
{
  uint32 i;
  for (i=0;i<MAX_MEMORY_MAPS;++i)
  {
    if (!mbr.memory_maps[i].used)
      break;
  }
  return i;
}

uint32 ArchCommon::getUsableMemoryRegion(size_t region, pointer &start_address, pointer &end_address, size_t &type)
{
  if (region >= MAX_MEMORY_MAPS)
    return 1;

  start_address = mbr.memory_maps[region].start_address;
  end_address = mbr.memory_maps[region].end_address;
  type = mbr.memory_maps[region].type;

  return 0;
}

#define MEMCOPY_LARGE_TYPE uint64

void ArchCommon::memcpy(pointer dest, pointer src, size_t size)
{
  MEMCOPY_LARGE_TYPE *s64 = (MEMCOPY_LARGE_TYPE*)src;
  MEMCOPY_LARGE_TYPE *d64 = (MEMCOPY_LARGE_TYPE*)dest;

  uint32 i;
  uint32 num_64_bit_copies = size / (sizeof(MEMCOPY_LARGE_TYPE));
  uint32 num_8_bit_copies = size % (sizeof(MEMCOPY_LARGE_TYPE));

  for (i=0;i<num_64_bit_copies;++i)
  {
    *d64 = *s64;
    ++d64;
    ++s64;
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
  if (debug) kprintfd("Bzero start %x\n",s);
  MEMCOPY_LARGE_TYPE *s64 = (MEMCOPY_LARGE_TYPE*)s;
  uint32 num_64_bit_zeros = n / sizeof(MEMCOPY_LARGE_TYPE);
  uint32 num_8_bit_zeros = n % sizeof(MEMCOPY_LARGE_TYPE);
  uint32 i;
  if (debug) kprintfd("Bzero next %x\n", s64);
  for (i=0;i<num_64_bit_zeros;++i)
  {
    *s64 = 0;
    ++s64;
  }
  uint8 *s8 = (uint8*)s64;
  if (debug) kprintfd("Bzero middle %x\n", s8);
  for (i=0;i<num_8_bit_zeros;++i)
  {
    *s8 = 0;
    ++s8;
  }
  if (debug) kprintfd("Bzero end, %x\n", s8);
}

uint32 ArchCommon::checksumPage(uint32 physical_page_number, uint32 page_size)
{
  return ArchCommon::checksum((uint32*)ArchMemory::getIdentAddressOfPPN(physical_page_number),page_size / sizeof(uint32));
}

uint32 ArchCommon::checksum(uint32* src, uint32 count)
{
  uint32 poly = 0xEDB88320;
  int bit = 0, nbits = 32;
  uint32 res = 0xFFFFFFFF;

  for (uint32 i = 0; i < count; ++i)
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
}
