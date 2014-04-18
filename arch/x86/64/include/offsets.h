/**
 * @file offset.h
 *
 */

#ifndef _offsets_h_
#define _offsets_h_

#include "types.h"
#include "paging-definitions.h"

extern PageMapLevel4Entry kernel_page_map_level_4[];

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
#define PML4_KERNEL_ADDR VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4)
#define PML4_KERNEL_PAGE ((uint64)PML4_KERNEL_ADDR / PAGE_SIZE)




#endif
