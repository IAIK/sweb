/**
 * @file kmalloc.cpp
 */

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

void* krealloc(void * address, size_t size)
{
  return (void*) KernelMemoryManager::instance()->reallocateMemory((pointer)address, size);
}
