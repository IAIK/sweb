//----------------------------------------------------------------------
//   $Id: new.h,v 1.2 2005/04/22 17:21:40 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: new.h,v $
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
void* __builtin_new(unsigned long size);
void* __builtin_vec_new(unsigned long size);
void __builtin_vec_delete(void* address);





#endif
