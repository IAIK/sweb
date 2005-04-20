/**
 * $Id: offsets.h,v 1.2 2005/04/20 08:06:17 nomenquis Exp $
 *
 * $Log: offsets.h,v $
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


#endif
