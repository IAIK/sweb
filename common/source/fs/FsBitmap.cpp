/**
 * Filename: FsBitmap.cpp
 * Description:
 *
 * Created on: 16.05.2012
 * Author: Christopher Walles
 */

#include "fs/FsBitmap.h"
#include "fs/FileSystem.h"
#include "fs/FsVolumeManager.h"

FsBitmap::FsBitmap(FileSystem* file_system, FsVolumeManager* fs_volume_manager,
    sector_addr_t start_sector, sector_addr_t end_sector,
    bitmap_t num_bits) : file_system_(file_system),
    volume_manager_(fs_volume_manager),
    start_sector_(start_sector), num_bits_(num_bits), num_blocks_(end_sector+1 - start_sector)
{
  // check capacity
  assert((end_sector+1 - start_sector) * file_system_->getBlockSize() * 8 >= num_bits);
}

FsBitmap::~FsBitmap()
{
}

bool FsBitmap::setBit(bitmap_t index, bool value)
{
  if(index > num_bits_)
    return false;

  sector_addr_t sector = index / (8 * file_system_->getBlockSize() ) + start_sector_;
  sector_len_t offset = index % (8 * file_system_->getBlockSize() );

  sector_len_t buf_idx = offset / 8;
  uint8  bit     = offset % 8;

  volume_manager_->acquireSectorForWriting(sector);

  char* buffer = volume_manager_->readSectorUnprotected(sector);

  // setting the bit
  if(value)
    buffer[buf_idx] |= (1<<bit);
  else
    buffer[buf_idx] &= ~(1<<bit);

  // writing back to the FileSystem
  volume_manager_->writeSectorUnprotected(sector, buffer);

  // release lock
  volume_manager_->releaseWriteSector(sector);

  return true;
}

bool FsBitmap::getBit(bitmap_t index)
{
  debug(FS_BITMAP, "getBit - CALL bit=%d\n", index);

  if(index > num_bits_)
  {
    debug(FS_BITMAP, "getBit - ERROR index (%d) > num_bits_ (%d)\n", index, num_bits_);
    return false;
  }

  sector_len_t block_size = file_system_->getBlockSize();

  sector_addr_t sector = index / (8 * block_size) + start_sector_;
  sector_len_t offset = index % (8 * block_size);

  sector_len_t buf_idx = offset / 8;
  uint8       bit      = offset % 8;

  debug(FS_BITMAP, "getBit - sector=%x offset=%x buf_idx=%d bit=%d\n", sector, offset, buf_idx, bit);

  volume_manager_->acquireSectorForReading(sector);

  char* buffer = volume_manager_->readSectorUnprotected(sector);

  // setting the bit
  bool bit_set = false;
  (buffer[buf_idx] & (1<<bit)) ? bit_set = true : bit_set = false;

  debug(FS_BITMAP, "getBit - buffer[%d]=%x\n", buf_idx, buffer[buf_idx]);

  // release lock
  volume_manager_->releaseReadSector(sector);

  return bit_set;
}

bitmap_t FsBitmap::occupyNextFreeBit(void)
{
  debug(FS_BITMAP, "occupyNextFreeBit - finding a free Bit in the bitmap\n");
  bitmap_t next_free_bit = 0;

  sector_len_t block_size = file_system_->getBlockSize();

  debug(FS_BITMAP, "occupyNextFreeBit - bitmap uses %d blocks.\n", num_blocks_);

  // the current bit
  bitmap_t cur_bit = 0;

  for(sector_addr_t cur_block = 0; cur_block < num_blocks_; cur_block++)
  {
    volume_manager_->acquireSectorForWriting(start_sector_ + cur_block);

    char* buffer = volume_manager_->readSectorUnprotected(start_sector_ + cur_block);
    assert(buffer != NULL);

    for(sector_len_t i = 0; i < block_size; i++)
    {
      if(cur_bit >= num_bits_)
      {
        debug(FS_BITMAP, "occupyNextFreeBit - last bit is reached, quit.\n");
        break;
      }

      for(uint8 j = 0; j < 8; j++, cur_bit++)
      {
        //if(!(buffer[i] & (0xFF & (1<<j))))
        if(!(buffer[i] & (1<<j)))
        {
          // this bit is free!
          buffer[i] |= (1<<j);
          next_free_bit = cur_bit; //i*8 + j;

          // rewrite disk-block
          debug(FS_BITMAP, "occupyNextFreeBit - found a free bit (%d), block-no=%x block-offset=%d bit-no=%d.\n", next_free_bit, start_sector_+cur_block, i, j);
          volume_manager_->writeSectorUnprotected(start_sector_ + cur_block, buffer);
          break;
        }
      }

      if(next_free_bit != 0)
        break;
    }

    volume_manager_->releaseWriteSector(start_sector_ + cur_block);

    if(next_free_bit != 0)
    {
      debug(FS_BITMAP, "occupyNextFreeBit - free bit (%d), occupied!\n", next_free_bit);
      return next_free_bit;
    }
  }

  // Bitmap is completely full... sorry
  debug(FS_BITMAP, "occupyNextFreeBit - ERROR no more free bits.\n");
  return -1;
}

bitmap_t FsBitmap::getNumFreeBits(void) const
{
  debug(FS_BITMAP, "getNumFreeBits - CALL determining number of free bits.\n");

  sector_len_t block_size = file_system_->getBlockSize();

  bitmap_t cur_bit = 0;

  // number of free bits
  bitmap_t num_free_bits = 0;

  for(sector_addr_t cur_block = 0; cur_block < num_blocks_; cur_block++)
  {
    volume_manager_->acquireSectorForReading(start_sector_ + cur_block);

    char* buffer = volume_manager_->readSectorUnprotected(start_sector_ + cur_block);

    for(sector_len_t i = 0; i < block_size; i++)
    {

      for(uint8 j = 0; j < 8; j++, cur_bit++)
      {
        if(cur_bit >= num_bits_)
        {
          debug(FS_BITMAP, "getNumFreeBits - DONE Bitmap has %d free bits.\n", num_free_bits);

          volume_manager_->releaseReadSector(start_sector_ + cur_block);
          return num_free_bits;
        }

        if(!(buffer[i] & (1<<j)))
        {
          // this bit is free!
          num_free_bits++;
        }
      }

    }

    volume_manager_->releaseReadSector(start_sector_ + cur_block);
  }

  debug(FS_BITMAP, "getNumFreeBits - DONE Bitmap has %d free bits.\n", num_free_bits);
  return num_free_bits;
}
