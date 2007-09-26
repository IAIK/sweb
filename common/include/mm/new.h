/**
 * @file new.h
 */

#ifndef _NEW_H_
#define _NEW_H_

#include "types.h"
#include "kmalloc.h"

/**
 * wrapper for placement new operator
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
