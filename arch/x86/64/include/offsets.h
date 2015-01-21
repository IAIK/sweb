/**
 * @file offset.h
 *
 */

#ifndef _offsets_h_
#define _offsets_h_

#include "types.h"
#include "paging-definitions.h"

 /**
  * These are the basic offsets for our memory layout
  */

/**
 * this is the difference between link and load base
 */
#define PHYSICAL_TO_VIRTUAL_OFFSET 0xFFFFFFFF80000000ULL

/**
 * returns the physical address of a virtual address by using the offset
 */
#define VIRTUAL_TO_PHYSICAL_BOOT(x) ((void*)(~PHYSICAL_TO_VIRTUAL_OFFSET & ((uint64)x)))

#endif
