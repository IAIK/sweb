//----------------------------------------------------------------------
//   $Id: ArchCommon.cpp,v 1.3 2005/04/22 18:23:03 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: ArchCommon.cpp,v $
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

struct multiboot_remainder
{
uint32 memory_size;
uint32 vesa_x_res;
uint32 vesa_y_res;
uint32 vesa_bits_per_pixel;
uint32 have_vesa_console;
pointer vesa_lfb_pointer;
};

extern multiboot_info_t multi_boot_structure_pointer[];
static struct multiboot_remainder mbr = {0,0,0,0,0,0};
  
extern "C" void parseMultibootHeader();

void parseMultibootHeader()
{
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

uint32 ArchCommon::getVESAConsoleLFBPtr(uint32 is_paging_set_up)
{
  if (is_paging_set_up)
    return 1024U*1024U*1024U*3U - 1024U*1024U*16U;
  else
  {
    struct multiboot_remainder &orig_mbr = (struct multiboot_remainder &)(*((struct multiboot_remainder*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&mbr)));
    return orig_mbr.vesa_lfb_pointer;
  }
}

uint32 ArchCommon::getVESAConsoleBitsPerPixel()
{
  return mbr.vesa_bits_per_pixel;
}
