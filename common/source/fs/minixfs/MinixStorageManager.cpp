#include "fs/minixfs/MinixStorageManager.h"
#include "fs/minixfs/MinixFSSuperblock.h"
#include "assert.h"
#include "../../../include/console/kprintf.h"


MinixStorageManager::MinixStorageManager(Buffer *bm_buffer, uint16 num_inode_bm_blocks, uint16 num_zone_bm_blocks, uint16 num_inodes, uint16 num_zones) : StorageManager(num_inodes, num_zones)
{
  num_inode_bm_blocks_ = num_inode_bm_blocks;
  num_zone_bm_blocks_ = num_zone_bm_blocks;
  kprintfd( "StorageManager: num_inodes:%d\tnum_inode_bm_blocks:%d\tnum_zones:%d\tnum_zone_bm_blocks:%d\t\n",num_inodes,num_inode_bm_blocks,num_zones,num_zone_bm_blocks);
  bm_buffer->print();
  //read inode bitmap
  uint32 i_byte = 0;
  for (; i_byte < num_inodes / 8; i_byte ++)
  {
    inode_bitmap_->setByte(i_byte, bm_buffer->getByte(i_byte));
  }
  for (uint32 i_bit = 0; i_bit < num_inodes % 8; i_bit ++)
  {
    uint8 byte = bm_buffer->getByte(i_byte);
    if((byte >> i_bit) & 0x01)
      inode_bitmap_->setBit(i_byte * 8 + i_bit);
  }
  //read zone bitmap
  uint32 z_byte = num_inode_bm_blocks * MINIX_BLOCK_SIZE;
  for (; z_byte < num_zones / 8; z_byte ++)
  {
    zone_bitmap_->setByte(z_byte, bm_buffer->getByte(z_byte));
  }
  for (uint32 z_bit = 0; z_bit < num_zones % 8; z_bit ++)
  {    
    uint8 byte = bm_buffer->getByte(z_byte);
    if((byte >> z_bit) & 0x01)
      zone_bitmap_->setBit(z_byte * 8 + z_bit);
  }
  
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

size_t MinixStorageManager::acquireInode()
{
  size_t pos = curr_inode_pos_ + 1;
  for (;pos != curr_inode_pos_; pos++ )
  {
    if(pos >= inode_bitmap_->getSize())
      pos =0;
    if(!inode_bitmap_->getBit(pos))
    {
      inode_bitmap_->setBit(pos);
      curr_inode_pos_ = pos;
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

void MinixStorageManager::flush(MinixFSSuperblock *superblock)
{
  Buffer* bm_buffer = new Buffer((num_inode_bm_blocks_ + num_zone_bm_blocks_) * MINIX_BLOCK_SIZE);
  uint32 num_inodes = inode_bitmap_->getSize();
  uint32 i_byte = 0;
  for(; i_byte < num_inodes / 8; i_byte++)
  {
    bm_buffer->setByte(i_byte, inode_bitmap_->getByte(i_byte));
  }
  uint8 byte = 0;
  for(uint32 i_bit = 0; i_bit < 8; i_bit++)
  {
    if(i_bit < num_inodes % 8)
    {
      if( inode_bitmap_->getBit(i_bit))
      {
        byte &= 0x01 << i_bit;
      }
    }
    else
      byte &= 0x01 << i_bit;
  }
  bm_buffer->setByte(i_byte, byte);
  ++i_byte;
  for(; i_byte < num_inode_bm_blocks_ * MINIX_BLOCK_SIZE; i_byte++)
  {
    bm_buffer->setByte(i_byte, 0xff);
  }
  
  uint32 num_zones = zone_bitmap_->getSize();
  uint32 z_byte = i_byte;
  for(; z_byte < num_zones / 8; z_byte++)
  {
    bm_buffer->setByte(z_byte, zone_bitmap_->getByte(z_byte));
  }
  byte = 0;
  for(uint32 z_bit = 0; z_bit < 8; z_bit++)
  {
    if(z_bit < num_inodes % 8)
    {
      if( zone_bitmap_->getBit(z_bit))
      {
        byte &= 0x01 << z_bit;
      }
    }
    else
      byte &= 0x01 << z_bit;
  }
  bm_buffer->setByte(z_byte, byte);
  ++z_byte;
  for(; z_byte < num_inode_bm_blocks_ * MINIX_BLOCK_SIZE; z_byte++)
  {
    bm_buffer->setByte(z_byte, 0xff);
  }
  superblock->writeBlocks(2,num_inode_bm_blocks_ + num_zone_bm_blocks_, bm_buffer);
}

void MinixStorageManager::printBitmap()
{
  inode_bitmap_->bmprint();
}
