//----------------------------------------------------------------------
//   $Id: kmalloc.h,v 1.3 2005/04/26 15:49:07 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: kmalloc.h,v $
//  Revision 1.2  2005/04/22 17:21:40  nomenquis
//  added TONS of stuff, changed ZILLIONS of things
//
//  Revision 1.1  2005/04/20 21:35:33  nomenquis
//  started to implement page manager
//
//----------------------------------------------------------------------

#ifndef _KMALLOC_H_
#define _KMALLOC_H_

#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif
void *kmalloc(size_t size);
void kfree(void * address);
void *krealloc(void * address, size_t size);
#ifdef __cplusplus
}
#endif


#endif
