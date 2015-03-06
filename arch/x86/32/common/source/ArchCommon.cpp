#include "ArchCommon.h"
#include "multiboot.h"
#include "offsets.h"
#include "kprintf.h"
#include "ArchMemory.h"
#include "TextConsole.h"
#include "FrameBufferConsole.h"
#include "backtrace.h"
#include "Stabs2DebugInfo.h"
#include "ports.h"

extern void* kernel_end_address;

multiboot_info_t* multi_boot_structure_pointer = (multiboot_info_t*)0xDEADDEAD; // must not be in bss segment
static struct multiboot_remainder mbr __attribute__ ((section (".data"))); // must not be in bss segment

extern "C" void parseMultibootHeader()
{
  uint32 i;

  multiboot_info_t *mb_infos = *(multiboot_info_t**)VIRTUAL_TO_PHYSICAL_BOOT( (pointer)&multi_boot_structure_pointer);

  struct multiboot_remainder &orig_mbr = (struct multiboot_remainder &)(*((struct multiboot_remainder*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&mbr)));

  if (mb_infos && (mb_infos->flags & 1<<11))
  {
    struct vbe_mode* mode_info = (struct vbe_mode*)mb_infos->vbe_mode_info;
    orig_mbr.have_vesa_console = 1;
    orig_mbr.vesa_lfb_pointer = mode_info->phys_base;
    orig_mbr.vesa_x_res = mode_info->x_resolution;
    orig_mbr.vesa_y_res = mode_info->y_resolution;
    orig_mbr.vesa_bits_per_pixel = mode_info->bits_per_pixel;
  }

  if (mb_infos && (mb_infos->flags & 1<<3))
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

  if (mb_infos && (mb_infos->flags & 1<<6))
  {
    uint32 mmap_size = sizeof(memory_map);
    uint32 mmap_total_size = mb_infos->mmap_length;
    uint32 num_maps = mmap_total_size / mmap_size;

    for (i=0;i<num_maps;++i)
    {
      memory_map * map = (memory_map*)(mb_infos->mmap_addr+mmap_size*i);
      if(map->base_addr_high == 0)
      {
        orig_mbr.memory_maps[i].used = 1;
        orig_mbr.memory_maps[i].start_address = map->base_addr_low;
        orig_mbr.memory_maps[i].end_address = map->base_addr_low + map->length_low;
        orig_mbr.memory_maps[i].type = map->type;
      }
      else
      {
        orig_mbr.memory_maps[i].used = 0;
      }
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
   return (pointer)(1024U*1024U*1024U*2U + 1024U*1024U*4U); //2GB+4MB Ende des Kernel Bereichs fuer den es derzeit Paging gibt
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

Console* ArchCommon::createConsole(uint32 count)
{
  // deactivate cursor
  outportb(0x3d4, 0xa);
  outportb(0x3d5, 0b00100000);
  if (haveVESAConsole())
    return new FrameBufferConsole(count);
  else
    return new TextConsole(count);
}

Stabs2DebugInfo const *kernel_debug_info = 0;

void ArchCommon::initDebug()
{
  extern unsigned char stab_start_address_nr;
  extern unsigned char stab_end_address_nr;

  extern unsigned char stabstr_start_address_nr;

  kernel_debug_info = new Stabs2DebugInfo((char const *)&stab_start_address_nr,
                                          (char const *)&stab_end_address_nr,
                                          (char const *)&stabstr_start_address_nr);

}

void ArchCommon::idle()
{
  asm volatile("hlt");
}

void ArchCommon::drawHeartBeat()
{
  const char* clock = "/-\\|";
  static uint32 heart_beat_value = 0;
  char* fb = (char*)getFBPtr();
  fb[0] = clock[heart_beat_value++ % 4];
  fb[1] = 0x9f;
}
