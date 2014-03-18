/**
 * @file offset.h
 *
 */

#ifndef _offsets_h_
#define _offsets_h_

 /**
  * These are the basic offsets for our memory layout
  */


/**
 * Our image will be at 2gig virtual
 */
#define LINK_BASE 0x80000000 

/**
 * Our image will be at 1meg physical
 */
#define LOAD_BASE 0x00100000

/**
 * this is the difference between link and load base
 */
#define PHYSICAL_TO_VIRTUAL_OFFSET (LINK_BASE - LOAD_BASE)

/**
 * returns the virtual address of a physical address by using the offset
 */
#define PHYSICAL_TO_VIRTUAL_BOOT(x) ((x) + PHYSICAL_TO_VIRTUAL_OFFSET)

/**
 * returns the physical address of a virtual address by using the offset
 */
#define VIRTUAL_TO_PHYSICAL_BOOT(x) ((x) - PHYSICAL_TO_VIRTUAL_OFFSET)


#endif
