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
   * updates the mappings so the kernel is ready for use
   */
  extern uint64 updateBootTimePaging();

#ifdef __cplusplus
}
#endif


#endif
