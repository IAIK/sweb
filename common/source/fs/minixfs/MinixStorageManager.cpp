#include "fs/minixfs/MinixStorageManager.h"
#include "assert.h"


MinixStorageManager::MinixStorageManager(char *bm_buffer, uint16 num_inode_bm_blocks, uint16 num_zone_bm_blocks, uint16 num_inodes, uint16 num_zones) : StorageManager(num_inodes, num_zones)
{
  assert(bm_buffer);
}
MinixStorageManager::~MinixStorageManager()
{}

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
