//----------------------------------------------------------------------
//   $Id: ArchCommon.cpp,v 1.2 2005/04/22 17:40:57 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: ArchCommon.cpp,v $
//  Revision 1.1  2005/04/22 17:22:20  nomenquis
//  forgot this little sucker
//
//----------------------------------------------------------------------


#include "ArchCommon.h"
#include "multiboot.h"
#include "boot-time.h"
#include "offsets.h"

static uint32 memory_size=0;
static uint32 vesa_x_res=0;
static uint32 vesa_y_res=0;
static uint32 vesa_bits_per_pixel=0;
static uint32 have_vesa_console=0;
static pointer vesa_lfb_pointer=0;

extern "C" void parseMultibootHeader();

void parseMultibootHeader()
{
  extern multiboot_info_t multi_boot_structure_pointer[];
  multiboot_info_t *mb_infos = *(multiboot_info_t**)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&multi_boot_structure_pointer);
  if (mb_infos && mb_infos->flags & 1<<11)
  {
    struct vbe_mode* mode_info = (struct vbe_mode*)mb_infos->vbe_mode_info;
    have_vesa_console = 1;
    vesa_lfb_pointer = mode_info->phys_base;
    vesa_x_res = mode_info->x_resolution;
    vesa_y_res = mode_info->y_resolution;
    vesa_bits_per_pixel = mode_info->bits_per_pixel;
  }
}


uint32 ArchCommon::haveVESAConsole()
{
  return have_vesa_console;
}

uint32 ArchCommon::getVESAConsoleHeight()
{
  return vesa_y_res;
}

uint32 ArchCommon::getVESAConsoleWidth()
{
  return vesa_x_res;
}

uint32 ArchCommon::getVESAConsoleLFBPtr()
{
  return vesa_lfb_pointer;
}

uint32 ArchCommon::getVESAConsoleBitsPerPixel()
{
  return vesa_bits_per_pixel;
}
