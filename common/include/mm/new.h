//----------------------------------------------------------------------
//   $Id: new.h,v 1.1 2005/04/20 21:35:33 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: $
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







#endif
