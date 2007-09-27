/**
 * @file ArchCommon.h
 *
 * Collection of architecture dependent stuff
 * + FrameBuffer Info
 * + Grub Info (usable memory regions, modules)
 * + memcpy
 * + bzero
 */

#ifndef _ARCH_COMMON_H_
#define _ARCH_COMMON_H_

#include "types.h"


class ArchCommon
{
public:

  /** 
   * @return returns the end address of the kernel
   *
   */
  static pointer getKernelEndAddress();

  /** 
   * @return returns the start address of the free memory block used
   * for dynamic kernel memory
   *
   */
  static pointer getFreeKernelMemoryStart();

  /** 
   * @return returns the end address of the free memory block used
   * for dynamic kernel memory
   *
   */
  static pointer getFreeKernelMemoryEnd();

  /** 
   * @param is_paging_set_up inform function that paging is not yet up (=false) and
   * that it should behave accordingly
   * if is_paging_set_up =1 default
   *
   */
  static uint32 haveVESAConsole(uint32 is_paging_set_up = 1);

  /**
   * @return the hight of the VESA Console
   *
   */
  static uint32 getVESAConsoleHeight();

  /** 
   * @return the width of the VESA Console
   *
   */
  static uint32 getVESAConsoleWidth();

 /** 
  * @return the bits per pixel of the VESA Console
  *
  */
  static uint32 getVESAConsoleBitsPerPixel();

  /** 
   * @param is_paging_set_up 
   *   on false: returns memory address in real memory
   *   on true (default): returns mapped memory address
   * @return a Pointer to the location of the VESA Memory Region
   */
  static pointer getVESAConsoleLFBPtr(uint32 is_paging_set_up=1);

  /**
   * returns a Pointer to the location of the FrameBuffer
   *
   * @param is_paging_set_up 
   *   on false: returns memory address in real memory
   *   on true (default): returns mapped memory address
   * @return a Pointer to the location of the FrameBuffer
   */
  static pointer getFBPtr(uint32 is_paging_set_up = 1);


  /** 
   * tells you the number of Useable Memory Regions just as grub told us
   * @return number of Useable Memory Regions
   */
  static uint32 getNumUseableMemoryRegions();

  /**
   * Reads the Grub MultiBoot-Info and
   * returns the start- and end-address of the Useable Memory Region x
   *
   * @param number of the region-information to parse
   * @param &start_address of Useable Memory Region
   * @param &end_adress of Useable Memory Region
   * @param &type of Useable Memory Region
   * @return 1 if region >= number_of_regions, 0 otherwise
   */
  static uint32 getUsableMemoryRegion(uint32 region, pointer &start_address, pointer &end_address, uint32 &type);


  /** 
   * Copy Memory, fast
   *
   * @param dest Destination to copy to
   * @param src Source to read from
   * @param size Number of Bytes to copy
   */
  static void memcpy(pointer dest, pointer src, size_t size);

  /** 
   * Sets the first n bytes of the memory area starting at s to zero.
   *
   * @param s (start zeroing here)
   * @param n (zero n bytes)
   * @param debug (set to 1 for debugging info)
   */
  static void bzero(pointer s, size_t n, uint32 debug = 0);


  /** 
   * Parses the Grub MultiBoot Info with regard to modules
   *
   * @param is_paging_set_up set (default=1) set 0 if paging is not yet up
   * @return uint32 returns the number of modules loaded by grub
   */
  static uint32 getNumModules(uint32 is_paging_set_up = 1);

  /**
   * Parses the Grub MultiBoot Info with regard to modules
   *
   * @param num the number of grub-loaded module which this is about
   * @param is_paging_set_up set (default=1) set 0 if paging is not yet up
   * @return uint32 returns memory start address of module "num"
   */
  static uint32 getModuleStartAddress(uint32 num,uint32 is_paging_set_up = 1);

  /**
   * Parses the Grub MultiBoot Info with regard to modules
   *
   * @param num the number of grub-loaded module which this is about
   * @param is_paging_set_up set (default=1) set 0 if paging is not yet up
   * @return uint32 returns memory end address of module "num"
   */
  static uint32 getModuleEndAddress(uint32 num,uint32 is_paging_set_up = 1);

  /**
   * Parses the Grub MultiBoot Info with regard to modules
   * Used for debugging
   *
   * @param i
   * @param used
   * @param start
   * @param end
   */
  static void dummdumm(uint32 i, uint32 &used, uint32 &start, uint32 &end);

};

#endif
