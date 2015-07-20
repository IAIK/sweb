/**
 * @file kmalloc.cpp
 */

#include "kmalloc.h"
#include "KernelMemoryManager.h"
#include "backtrace.h"

void* kmalloc(size_t size)
{
  return (void*)KernelMemoryManager::instance()->allocateMemory(size);
}

void kfree(void * address)
{
  KernelMemoryManager::instance()->freeMemory((pointer)address, getCalledBefore(1));
}

void* krealloc(void * address, size_t size)
{
  return (void*) KernelMemoryManager::instance()->reallocateMemory((pointer)address, size, getCalledBefore(1));
}
