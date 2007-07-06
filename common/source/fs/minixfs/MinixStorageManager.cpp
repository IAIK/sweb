#include "fs/minixfs/MinixStorageManager.h"
#include "assert.h"


MinixStorageManager::MinixStorageManager(Buffer *bm_buffer, uint16 num_inode_bm_blocks, uint16 num_zone_bm_blocks, uint16 num_inodes, uint16 num_zones) : StorageManager(num_inodes, num_zones)
{
  uint32 i_byte = 0;
  for (; i_byte < num_inodes / 8; i_byte ++)
  {
    inode_bitmap_->setByte(i_byte, bm_buffer->getByte(i_byte));
  }
  for (uint32 i_bit = 0; i_bit < num_inodes % 8; i_bit ++)
  {
    inode_bitmap_->setBit(i_byte * 8 + i_bit);
  }
  curr_zone_pos_ = 0;
  //TODO read zones!!!
}

MinixStorageManager::~MinixStorageManager()
{
  delete inode_bitmap_;
  delete zone_bitmap_;
}


bool MinixStorageManager::isInodeSet(size_t index)
{
  return inode_bitmap_->getBit(index);
}

uint32 MinixStorageManager::getNumUsedInodes()
{
  return inode_bitmap_->getNumBitsSet();
}


size_t MinixStorageManager::acquireZone()
{
  size_t pos = curr_zone_pos_ + 1;
  for (;pos != curr_zone_pos_; pos++ )
  {
    if(pos >= zone_bitmap_->getSize())
      pos =0;
    if(!zone_bitmap_->getBit(pos))
    {
      zone_bitmap_->setBit(pos);
      curr_zone_pos_ = pos;
      return pos;
    }
  }
  assert(false); // full memory should have been checked.
  return 0;
}

void MinixStorageManager::freeZone(size_t index)
{
  zone_bitmap_->unsetBit(index);
}
