#include "StorageManager.h"


StorageManager::StorageManager(uint16 num_inodes, uint16 num_zones)
{
  inode_bitmap_ = new Bitmap(num_inodes);
  zone_bitmap_ = new Bitmap(num_zones);
}

StorageManager::~StorageManager()
{
  delete inode_bitmap_;
  delete zone_bitmap_;
}

