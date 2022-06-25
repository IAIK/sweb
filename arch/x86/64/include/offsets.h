#pragma once

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

#define TRUNCATE(X) ({ volatile unsigned int x = (unsigned int)(((char*)X)+0x7FFFFFFF); (char*)(x+1); })

/**
 * Use only the lower canonical half for userspace
 */
#define USER_BREAK 0x0000800000000000ULL

/**
 * End of the non-canonical space, start of kernel space
 */
#define KERNEL_START 0xffff800000000000ULL

#define IDENT_MAPPING_START 0xFFFFF00000000000ULL
