//----------------------------------------------------------------------
//   $Id: kmalloc.cpp,v 1.2 2005/04/23 17:35:03 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: kmalloc.cpp,v $
//  Revision 1.1  2005/04/22 17:21:41  nomenquis
//  added TONS of stuff, changed ZILLIONS of things
//
//----------------------------------------------------------------------

#include "kmalloc.h"
#include "KernelMemoryManager.h"

void* kmalloc(size_t size)
{
  return (void*)KernelMemoryManager::instance()->allocateMemory(size);
}

void kfree(void * address)
{
  KernelMemoryManager::instance()->freeMemory((pointer)address);
}
