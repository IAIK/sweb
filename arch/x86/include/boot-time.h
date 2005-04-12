/**
 * $Id: boot-time.h,v 1.1 2005/04/12 17:46:43 nomenquis Exp $
 *
 * $Log:  $
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
   * This one shall be called when you've already set up the correct and final
   * kernel mapping. Only call this one once after you've set up a new and final
   * mapping
   */
  extern void freeBootTimePaging();

#ifdef __cplusplus
}
#endif


#endif
