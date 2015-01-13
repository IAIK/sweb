/**
 * @file boot-time.h
 *
 */

#ifndef _BOOT_TIME_H_
#define _BOOT_TIME_H_

#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * This one is called from the bootup asm stuff
   * It's used to set up the initial paging for the 2gig kernel mapping
   */
  extern void initialiseBootTimePaging();

  /**
   * zeros the kernel_page_directory entries
   */
  extern void removeBootTimeIdentMapping();

#ifdef __cplusplus
}
#endif


#endif
