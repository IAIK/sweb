//----------------------------------------------------------------------
//   $Id: ArchCommon.cpp,v 1.8 2005/04/25 22:40:18 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: ArchCommon.cpp,v $
//  Revision 1.7  2005/04/25 21:15:41  nomenquis
//  lotsa changes
//
//  Revision 1.6  2005/04/23 18:13:26  nomenquis
//  added optimised memcpy and bzero
//  These still could be made way faster by using asm and using cache bypassing mov instructions
//
//  Revision 1.5  2005/04/23 15:58:31  nomenquis
//  lots of new stuff
//
//  Revision 1.4  2005/04/23 11:56:34  nomenquis
//  added interface for memory maps, it works now
//
//  Revision 1.2  2005/04/22 17:40:57  nomenquis
//  cleanup
//
//  Revision 1.1  2005/04/22 17:22:20  nomenquis
//  forgot this little sucker
//
//----------------------------------------------------------------------


#include "ArchCommon.h"
#include "multiboot.h"
#include "boot-time.h"
#include "offsets.h"

#define MAX_MEMORY_MAPS 10
#define FOUR_ZEROS 0,0,0,0
#define EIGHT_ZEROS FOUR_ZEROS,FOUR_ZEROS
#define SIXTEEN_ZEROS EIGHT_ZEROS,EIGHT_ZEROS
#define TWENTY_ZEROS SIXTEEN_ZEROS,FOUR_ZEROS
#define FOURTY_ZEROS TWENTY_ZEROS,TWENTY_ZEROS

struct multiboot_remainder
{
uint32 memory_size;
uint32 vesa_x_res;
uint32 vesa_y_res;
uint32 vesa_bits_per_pixel;
uint32 have_vesa_console;
pointer vesa_lfb_pointer;

  struct memory_maps
  {
    uint32 used;
    pointer start_address;
    pointer end_address;
    uint32 type;
  } memory_maps[MAX_MEMORY_MAPS];
};


extern multiboot_info_t multi_boot_structure_pointer[];
static struct multiboot_remainder mbr = {0,0,0,0,0,0,FOURTY_ZEROS};
  
extern "C" void parseMultibootHeader();


#define print(x)     fb_start += 2; \
    { \
      uint32 divisor; \
      uint32 current; \
      uint32 remainder; \
      current = (uint32)x; \
      divisor = 1000000000; \
      while (divisor > 0) \
      { \
        remainder = current % divisor; \
        current = current / divisor; \
        \
        fb[fb_start++] = (uint8)current + '0' ; \
        fb[fb_start++] = 0x9f ; \
    \
        divisor = divisor / 10; \
        current = remainder; \
      }      \
    }
    
void parseMultibootHeader()
{
  uint32 i;
  //uint8 * fb = (uint8*) 0x000B8000;
  multiboot_info_t *mb_infos = *(multiboot_info_t**)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&multi_boot_structure_pointer);
  struct multiboot_remainder &orig_mbr = (struct multiboot_remainder &)(*((struct multiboot_remainder*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&mbr)));
  if (mb_infos && mb_infos->flags & 1<<11)
  {
    struct vbe_mode* mode_info = (struct vbe_mode*)mb_infos->vbe_mode_info;
    orig_mbr.have_vesa_console = 1;
    orig_mbr.vesa_lfb_pointer = mode_info->phys_base;
    orig_mbr.vesa_x_res = mode_info->x_resolution;
    orig_mbr.vesa_y_res = mode_info->y_resolution;
    orig_mbr.vesa_bits_per_pixel = mode_info->bits_per_pixel;
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
      memory_map * map = (memory_map*)(mb_infos->mmap_addr+mmap_size*i);
      orig_mbr.memory_maps[i].used = 1;
      orig_mbr.memory_maps[i].start_address = map->base_addr_low;
      orig_mbr.memory_maps[i].end_address = map->base_addr_low + map->length_low;
      orig_mbr.memory_maps[i].type = map->type;
    }
  }
  
  

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
    return 1024U*1024U*1024U*3U - 1024U*1024U*16U;
  else
  {
    struct multiboot_remainder &orig_mbr = (struct multiboot_remainder &)(*((struct multiboot_remainder*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&mbr)));
    return orig_mbr.vesa_lfb_pointer;
  }
}

pointer ArchCommon::getFBPtr(uint32 is_paging_set_up)
{
  if (is_paging_set_up)
    return 0xC00B8000;
  else
    return 0x000B8000;
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

uint32 ArchCommon::getUsableMemoryRegion(uint32 region, pointer &start_address, pointer &end_address, uint32 &type)
{
  if (region >= MAX_MEMORY_MAPS)
    return 1;
  
  start_address = mbr.memory_maps[region].start_address;
  end_address = mbr.memory_maps[region].end_address;
  type = mbr.memory_maps[region].type;
  
  return 0;
}

#define MEMCOPY_LARGE_TYPE uint32

void ArchCommon::memcpy(pointer dest, pointer src, size_t size)
{
  MEMCOPY_LARGE_TYPE *s64 = (MEMCOPY_LARGE_TYPE*)src;
  MEMCOPY_LARGE_TYPE *d64 = (MEMCOPY_LARGE_TYPE*)dest;
  
  uint32 i;
  uint32 num_64_bit_copies = size / sizeof(MEMCOPY_LARGE_TYPE);
  uint32 num_8_bit_copies = size % sizeof(MEMCOPY_LARGE_TYPE);
  
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
void ArchCommon::bzero(pointer s, size_t n)
{
  MEMCOPY_LARGE_TYPE *s64 = (MEMCOPY_LARGE_TYPE*)s;
  uint32 num_64_bit_zeros = n / sizeof(MEMCOPY_LARGE_TYPE);
  uint32 num_8_bit_zeros = n % sizeof(MEMCOPY_LARGE_TYPE);
  uint32 i;
  
  for (i=0;i<num_64_bit_zeros;++i)
  {
    *s64 = 0;
    ++s64;
  }
  uint8 *s8 = (uint8*)s64;
  
  for (i=0;i<num_8_bit_zeros;++i)
  {
    *s8 = 0;
    ++s8;
  }
}
