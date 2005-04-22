//----------------------------------------------------------------------
//   $Id: kmalloc.h,v 1.2 2005/04/22 17:21:40 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: kmalloc.h,v $
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
void* kmalloc(size_t size);
void kfree(void * address);
#ifdef __cplusplus
}
#endif


#endif
