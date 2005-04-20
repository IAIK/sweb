/**
 * $Id: offsets.h,v 1.3 2005/04/20 15:26:35 nomenquis Exp $
 *
 * $Log: offsets.h,v $
 * Revision 1.2  2005/04/20 08:06:17  nomenquis
 * the overloard (thats me) managed to get paging with 4m pages to work.
 * kernel is now at 2g +1 and writes something to the fb
 * w00t!
 *
 * Revision 1.1  2005/04/12 17:46:43  nomenquis
 * added lots of files
 *
 *
 */

#ifndef _offsets_h_
#define _offsets_h_

 /**
  * These are the basic offsets for our memory layout
  */

/** Our image will be at 2gig virtual */
#define LINK_BASE 0x80100000 

/** Our image will be at 1meg physical */
#define LOAD_BASE 0x00100000

/** this is the difference between link and load base */
#define PHYSICAL_TO_VIRTUAL_OFFSET (LINK_BASE - LOAD_BASE)


/** returns the virtual address of a physical address by using the offset*/
#define PHYSICAL_TO_VIRTUAL_BOOT(x) ((x) + PHYSICAL_TO_VIRTUAL_OFFSET)

/** returns the physical address of a virtual address by using the offset */
#define VIRTUAL_TO_PHYSICAL_BOOT(x) ((x) - PHYSICAL_TO_VIRTUAL_OFFSET)


#endif
