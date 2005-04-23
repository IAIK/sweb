//----------------------------------------------------------------------
//   $Id: new.h,v 1.5 2005/04/23 17:35:03 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: new.h,v $
//  Revision 1.4  2005/04/23 15:57:49  btittelbach
//  new mit kmm
//
//  Revision 1.3  2005/04/22 19:43:04  nomenquis
//   more poison added
//
//  Revision 1.2  2005/04/22 17:21:40  nomenquis
//  added TONS of stuff, changed ZILLIONS of things
//
//  Revision 1.1  2005/04/20 21:35:33  nomenquis
//  started to implement page manager
//
//----------------------------------------------------------------------

#ifndef _NEW_H_
#define _NEW_H_

#include "types.h"
#include "kmalloc.h"

/**
 * This little sucker is a wrapper for good ol' placement new
 */
inline void* operator new(size_t, void* p) 
{ 
  return p; 
}

void __builtin_delete(void* address);
void* __builtin_new(uint32 size);
void* __builtin_vec_new(uint32 size);
void __builtin_vec_delete(void* address);


#endif
