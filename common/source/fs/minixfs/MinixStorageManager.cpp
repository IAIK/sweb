#include "MinixStorageManager.h"

#include "MinixFSSuperblock.h"

#include "EASTL/unique_ptr.h"

#include "assert.h"
#include "debug.h"

MinixStorageManager::MinixStorageManager(char *bm_buffer, uint16 num_inode_bm_blocks, uint16 num_zone_bm_blocks, uint16 num_inodes, uint16 num_zones) :
    StorageManager(num_inodes, num_zones)
{
  debug(M_STORAGE_MANAGER,
        "Constructor: num_inodes:%d\tnum_inode_bm_blocks:%d\tnum_zones:%d\tnum_zone_bm_blocks:%d\t\n", num_inodes, num_inode_bm_blocks, num_zones, num_zone_bm_blocks);

  num_inode_bm_blocks_ = num_inode_bm_blocks;
  num_zone_bm_blocks_ = num_zone_bm_blocks;

  uint32 i_byte = 0;
  for (; i_byte < num_inodes / 8; i_byte++)
  {
    inode_bitmap_.setByte(i_byte, bm_buffer[i_byte]);
  }
  for (uint32 i_bit = 0; i_bit < num_inodes % 8; i_bit++)
  {
    uint8 byte = bm_buffer[i_byte];
    if ((byte >> i_bit) & 0x01)
      inode_bitmap_.setBit(i_byte * 8 + i_bit);
  }
  //read zone bitmap
  uint32 z_byte = num_inode_bm_blocks * BLOCK_SIZE;
  uint32 z_bm_byte = 0;
  for (; z_bm_byte < num_zones / 8; z_byte++, z_bm_byte++)
  {
    zone_bitmap_.setByte(z_bm_byte, bm_buffer[z_byte]);
  }
  for (uint32 z_bit = 0; z_bit < num_zones % 8; z_bit++)
  {
    uint8 byte = bm_buffer[z_byte];
    if ((byte >> z_bit) & 0x01)
      zone_bitmap_.setBit(z_bm_byte * 8 + z_bit);
  }
  curr_inode_pos_ = 0;
  curr_zone_pos_ = 0;
}

MinixStorageManager::~MinixStorageManager()
{
  debug(M_STORAGE_MANAGER, "Destructor: destroyed\n");
}

bool MinixStorageManager::isInodeSet(size_t index)
{
  assert(index < inode_bitmap_.getSize() && "MinixStorageManager::isInodeSet called with bad index number");
  return inode_bitmap_.getBit(index);
}

uint32 MinixStorageManager::getNumUsedInodes()
{
  return inode_bitmap_.getNumBitsSet();
}

size_t MinixStorageManager::allocZone()
{
  size_t pos = curr_zone_pos_ + 1;
  for (; pos != curr_zone_pos_; ++pos)
  {
    if (pos >= zone_bitmap_.getSize())
    {
      pos = 0;
      if(pos == curr_zone_pos_) // Increment happens before check in for loop, which would lead to an infinite loop when curr_zone_pos_ = 0
      {
              break;
      }
    }
    if (!zone_bitmap_.getBit(pos))
    {
      zone_bitmap_.setBit(pos);
      curr_zone_pos_ = pos;
      debug(M_STORAGE_MANAGER, "acquireZone: Zone %zu acquired\n", pos);
      return pos;
    }
  }
  debug(M_STORAGE_MANAGER, "acquireZone: NO FREE ZONE FOUND!\n");
  assert(false && "No free minixfs zone found"); // full memory should have been checked.
  return 0;
}

size_t MinixStorageManager::allocInode()
{
  size_t pos = curr_inode_pos_ + 1;
  for (; pos != curr_inode_pos_; pos++)
  {
    if (pos >= inode_bitmap_.getSize())
      pos = 0;
    if (!inode_bitmap_.getBit(pos))
    {
      inode_bitmap_.setBit(pos);
      curr_inode_pos_ = pos;
      debug(M_STORAGE_MANAGER, "acquireInode: Inode %zu acquired\n", pos);
      return pos;
    }
  }
  kprintfd("acquireInode: NO FREE INODE FOUND!\n");
  assert(false && "Unable to allocate new inode on minixfs, no free inode found"); // full memory should have been checked.
  return 0;
}

void MinixStorageManager::freeZone(size_t index)
{
  zone_bitmap_.unsetBit(index);
  debug(M_STORAGE_MANAGER, "freeZone: Zone %zu freed\n", index);
}

void MinixStorageManager::freeInode(size_t index)
{
  inode_bitmap_.unsetBit(index);
  debug(M_STORAGE_MANAGER, "freeInode: Inode %zu freed\n", index);
}

void MinixStorageManager::flush([[maybe_unused]]MinixFSSuperblock *superblock)
{
  debug(M_STORAGE_MANAGER, "flush: starting flushing\n");
  if (M_STORAGE_MANAGER & OUTPUT_ENABLED)
    printBitmap();

  auto buffer_block_size = num_inode_bm_blocks_ + num_zone_bm_blocks_;
  auto buffer_size = buffer_block_size * BLOCK_SIZE;

  auto bm_buffer = eastl::make_unique<char[]>(buffer_size);
  memset(bm_buffer.get(), 0, buffer_size);

  memcpy(bm_buffer.get(), inode_bitmap_.data(), inode_bitmap_.getBytesSize());
  memcpy(bm_buffer.get() + num_inode_bm_blocks_*BLOCK_SIZE, zone_bitmap_.data(), zone_bitmap_.getBytesSize());


  superblock->writeBlocks(2, buffer_block_size, bm_buffer.get());

  debug(M_STORAGE_MANAGER, "flush: flushing finished\n");
}

void MinixStorageManager::printBitmap()
{
  inode_bitmap_.bmprint();
  zone_bitmap_.bmprint();
}
