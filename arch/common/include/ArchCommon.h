//----------------------------------------------------------------------
//   $Id: ArchCommon.h,v 1.9 2005/09/20 17:42:56 lythien Exp $
//----------------------------------------------------------------------
//
//  $Log: ArchCommon.h,v $
//  Revision 1.8  2005/09/03 17:08:34  nomenquis
//  added support for grub modules
//
//  Revision 1.7  2005/04/27 08:58:16  nomenquis
//  locks work!
//  w00t !
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
//  Revision 1.3  2005/04/22 18:23:03  nomenquis
//  massive cleanups
//
//  Revision 1.2  2005/04/22 17:40:57  nomenquis
//  cleanup
//
//  Revision 1.1  2005/04/22 17:21:38  nomenquis
//  added TONS of stuff, changed ZILLIONS of things
//
//----------------------------------------------------------------------

#ifndef _ARCH_COMMON_H_
#define _ARCH_COMMON_H_

#include "types.h"

/** @class ArchCommon
 *
 * The class ArchCommon manages the size of the console
 * and the userbility of the console
 *
 */

class ArchCommon
{
public:

  /** @haveVESAConsole
   *
   * @param is_paging_set_up get a paramater
   * if is_paging_set_up =1 default
   *
   */
  static uint32 haveVESAConsole(uint32 is_paging_set_up=1);

  /** @getVESAConsoleHight
   *
   * gets the hight of the VESA Console
   *
   */
  static uint32 getVESAConsoleHeight();

  /** @getVESAConsoleWidth
   *
   * gets the width of the VESA Console
   *
   */
  static uint32 getVESAConsoleWidth();

 /** @getVESAConsoleBitsPerPixel
  *
  * gets the bits per pixel of the VESA Console
  *
  */
  static uint32 getVESAConsoleBitsPerPixel();

  /** @getVESAConsoleLFBPtr
   *
   * FIXXMEE
   * gets the FB-Pointer from the VESA Console
   *
   * @param is_paging_set_up set up the paging
   *
   */
  static pointer getVESAConsoleLFBPtr(uint32 is_paging_set_up=1);

  /** @getFBPtr
   * FBPTR framebuffer pointer
   *
   * @param is_paging_set_up set up the paging
   */
  static pointer getFBPtr(uint32 is_paging_set_up=1);


  /** @getNumUseableMemoryRegions
   *
   * gets the number of usable memory regions
   *
   */
  static uint32 getNumUseableMemoryRegions();

  /** @getUsableMemoryRegion
   *
   * @param region is the region of FIXXME
   * @param start_address
   * @param end_adress
   * @param type
   */
  static uint32 getUsableMemoryRegion(uint32 region, pointer &start_address, pointer &end_address, uint32 &type);


  /** @memcpy
   *
   *
   * @param dest
   * @param src
   * @param size
   */
  static void memcpy(pointer dest, pointer src, size_t size);

  /** @bzero
   * The  bzero()  function sets the first n bytes of the byte area starting at s to zero.
   *
   * @param s
   * @param n
   * @param debug
   */
  static void bzero(pointer s, size_t n, uint32 debug = 0);


  /** @getNumModules
   *
   *
   * @param is_paging_set_up set up the paging
   */
  static uint32 getNumModules(uint32 is_paging_set_up=1);

  /** @getModuleStartAddress
   *
   *
   * @param num
   * @param is_paging_set_up
   */
  static uint32 getModuleStartAddress(uint32 num,uint32 is_paging_set_up=1);

  /** @getModuleEndAddress
   *
   *
   * @param num
   * @param is_paging_set_up set up the paging
   */
  static uint32 getModuleEndAddress(uint32 num,uint32 is_paging_set_up=1);


  /** @dummdumm
   *
   *
   * @param i
   * @param used
   * @param start
   * @param end
   */
  static void dummdumm(uint32 i, uint32 &used, uint32 &start, uint32 &end);

};







#endif
