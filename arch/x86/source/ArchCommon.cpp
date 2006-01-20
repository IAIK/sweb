//----------------------------------------------------------------------
//   $Id: ArchCommon.cpp,v 1.16 2006/01/20 07:20:04 nightcreature Exp $
//----------------------------------------------------------------------
//
//  $Log: ArchCommon.cpp,v $
//  Revision 1.15  2005/09/26 15:29:05  btittelbach
//  check
//
//  Revision 1.14  2005/09/03 18:20:14  nomenquis
//  pseudo fs works now
//
//  Revision 1.12  2005/07/26 17:45:25  nomenquis
//  foobar
//
//  Revision 1.11  2005/04/27 09:19:20  nomenquis
//  only pack whats needed
//
//  Revision 1.10  2005/04/27 08:58:16  nomenquis
//  locks work!
//  w00t !
//
//  Revision 1.9  2005/04/26 15:58:45  nomenquis
//  threads, scheduler, happy day
//
//  Revision 1.8  2005/04/25 22:40:18  btittelbach
//  Anti Warnings v0.1
//
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
#include "kprintf.h"

#define MAX_MEMORY_MAPS 10
#define MAX_MODULE_MAPS 10
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
uint32 num_module_maps;
   
  struct memory_maps
  {
    uint32 used;
    pointer start_address;
    pointer end_address;
    uint32 type;
  } __attribute__((__packed__)) memory_maps[MAX_MEMORY_MAPS];
  
  struct module_maps
  {
    uint32 used;
    pointer start_address;
    pointer end_address;
    uint8 name[256];
  } __attribute__((__packed__)) module_maps[MAX_MODULE_MAPS];
    
  
}__attribute__((__packed__));


extern multiboot_info_t multi_boot_structure_pointer[];
static struct multiboot_remainder mbr = {0,0,0,0,0,0,FOURTY_ZEROS};
  
extern "C" void parseMultibootHeader();

/*
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
*/
void parseMultibootHeader()
{
  uint32 i;
  //~ uint32 fb_start = 0;
  //~ uint8 * fb = (uint8*) 0x000B8000;
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
  if (mb_infos && mb_infos->flags && 1<<3)
  {

     module_t * mods = (module_t*)mb_infos->mods_addr;
     for (i=0;i<mb_infos->mods_count;++i)
     {
        orig_mbr.module_maps[i].used = 1;
        orig_mbr.module_maps[i].start_address = mods[i].mod_start;
        orig_mbr.module_maps[i].end_address = mods[i].mod_end;
        //FIXXXME, copy module name
        
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
      memory_map * map = (memory_map*)(mb_infos->mmap_addr+mmap_size*i);
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
   return (pointer)(1024U*1024U*1024U*2U + 1024U*1024U*4U); //2GB+4MB Ende des Kernel Bereichs für den es derzeit Paging gibt
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

uint32 ArchCommon::getModuleStartAddress(uint32 num, uint32 is_paging_set_up)
{
  if (is_paging_set_up)
    return mbr.module_maps[num].start_address + 3*1024*1024*1024U;
  else
  {
    struct multiboot_remainder &orig_mbr = (struct multiboot_remainder &)(*((struct multiboot_remainder*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&mbr)));
    return orig_mbr.module_maps[num].start_address ;
  }

}

uint32 ArchCommon::getModuleEndAddress(uint32 num, uint32 is_paging_set_up)
{
  if (is_paging_set_up)
    return mbr.module_maps[num].end_address + 3*1024*1024*1024U;
  else
  {
    struct multiboot_remainder &orig_mbr = (struct multiboot_remainder &)(*((struct multiboot_remainder*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&mbr)));
    return orig_mbr.module_maps[num].end_address;
  }
 
}

void ArchCommon::dummdumm(uint32 i, uint32 &used, uint32 &start, uint32 &end)
{
   
   used = mbr.module_maps[i].used;
   start = mbr.module_maps[i].start_address;
   end = mbr.module_maps[i].end_address;
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
  if (debug) kprintf_nosleep("Bzero start\n");
  MEMCOPY_LARGE_TYPE *s64 = (MEMCOPY_LARGE_TYPE*)s;
  uint32 num_64_bit_zeros = n / sizeof(MEMCOPY_LARGE_TYPE);
  uint32 num_8_bit_zeros = n % sizeof(MEMCOPY_LARGE_TYPE);
  uint32 i;
  if (debug) kprintf_nosleep("Bzero next\n");
  for (i=0;i<num_64_bit_zeros;++i)
  {
    *s64 = 0;
    ++s64;
  }
  uint8 *s8 = (uint8*)s64;
  if (debug) kprintf_nosleep("Bzero middle\n");
  for (i=0;i<num_8_bit_zeros;++i)
  {
    *s8 = 0;
    ++s8;
  }
  if (debug) kprintf_nosleep("Bzero end\n");
}
