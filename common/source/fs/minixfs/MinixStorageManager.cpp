#include "fs/minixfs/MinixStorageManager.h"
#include "assert.h"

void *MinixStorageManager::allocateMemory(uint32 size)
{
  assert(size);
  assert(false);
  return (void *) 0;
}

void MinixStorageManager::freeMemory(void * data)
{
  assert(data);
  assert(false);
}
