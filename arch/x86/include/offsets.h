/**
 * $Id: offsets.h,v 1.1 2005/04/12 17:46:43 nomenquis Exp $
 *
 * $Log:  $
 *
 */

#ifndef _offsets_h_
#define _offsets_h_

 /**
  * These are the basic offsets for our memory layout
  */

/** Our image will be at 2gig virtual */
#define LINK_BASE 0x80000000 

/** Our image will be at 1meg physical */
#define LOAD_BASE 0x00100000

/** this is the difference between link and load base */
#define PHYSICAL_TO_VIRTUAL_OFFSET (LINK_BASE - LOAD_BASE)


#endif
