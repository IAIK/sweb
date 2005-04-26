//----------------------------------------------------------------------
//   $Id: kmalloc.cpp,v 1.3 2005/04/26 15:49:07 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: kmalloc.cpp,v $
//  Revision 1.2  2005/04/23 17:35:03  nomenquis
//  fixed buggy memory manager
//  (giving out the same memory several times is no good idea)
//
//  Revision 1.1  2005/04/22 17:21:41  nomenquis
//  added TONS of stuff, changed ZILLIONS of things
//
//----------------------------------------------------------------------

#include "kmalloc.h"
#include "KernelMemoryManager.h"

void* kmalloc(size_t size)
{
  return (void*)vKernelMemoryManager::instance()->allocateMemory(size);
}

void kfree(void * address)
{
  KernelMemoryManager::instance()->freeMemory((pointer)address);
}

void* krealloc(void * address, size_t size)
{
  return (void*) KernelMemoryManager::instance()->reallocateMemory((pointer)address, size);
}
