/**
 * Filename: FileSystemUnix.cpp
 * Description:
 *
 * Created on: 19.07.2012
 * Author: chris
 */

#include "fs/unixfs/FileSystemUnix.h"
#include "fs/unixfs/InodeTable.h"
#include "fs/unixfs/UnixInodeCache.h"

#include "fs/FsVolumeManager.h"

#ifdef USE_FILE_SYSTEM_ON_GUEST_OS
#include <cstring>
#include <math.h>
#include "debug_print.h"
#else
#include "kprintf.h"
#include "math.h"
#endif

FileSystemUnix::FileSystemUnix(FsDevice* device, uint32 mount_flags) : FileSystem(device, mount_flags),
    inode_table_(NULL)
{
}

FileSystemUnix::~FileSystemUnix()
{
  // delete InodeTable
  if(inode_table_ != NULL)
  {
    // flush inode-cache
    inode_cache_->flush();
    delete inode_table_;
  }
}

int32 FileSystemUnix::fsync(Inode* inode)
{
  int32 ret_val = FileSystem::fsync(inode);

  if(inode_cache_ != NULL)
  {
    UnixInodeIdent ident(inode->getID());
    if(inode_cache_->writeItemImmediately(ident))
      ret_val = 0;
  }

  return ret_val;
}

void FileSystemUnix::resolveIndirectDataBlocks(Inode* inode, sector_addr_t indirect_block, uint16 degree_of_indirection, uint16 sector_addr_len)
{
  if(indirect_block == UNUSED_DATA_BLOCK)
  {
    debug(FS_UNIX, "resolveIndirectDataBlocks - indirect_block is not in use - quit\n");
    return;
  }

  if(degree_of_indirection == 0)
  {
    debug(FS_UNIX, "resolveIndirectDataBlocks - sorry this method only works for indirect blocks - quit\n");
    return;
  }

  debug(FS_UNIX, "resolveIndirectDataBlocks - indirect_block %x deg_of_indirection=%d\n", indirect_block, degree_of_indirection);
  FileSystem* fs = inode->getFileSystem();
  assert(fs != NULL);
  FsVolumeManager* volume_manager = fs->getVolumeManager();
  assert(volume_manager != NULL);

  // lock sector for reading
  volume_manager->acquireDataBlockForReading(indirect_block);

  // readout sector data
  char* sector = volume_manager->readDataBlockUnprotected(indirect_block);

  // scan the current block for more indirect blocks
  for(sector_len_t i = 0; i < fs->getDataBlockSize(); i+= sector_addr_len)
  {
    sector_addr_t new_sector = 0;

    // reading the current sector entry in a very flexible way
    for(uint16 j = 0; j < sector_addr_len; j++)
    {
      //debug(FS_UNIX, "resolveIndirectDataBlocks - sector[%d + %d]=%x\n", i, j, (uint8)sector[i + j]);
      new_sector |= ((uint8)(sector[i + j])) << (8*j);
    }

    if(new_sector == UNUSED_DATA_BLOCK)
    {
      // from here on there are no more indirect blocks to resolve, we can quit
      debug(FS_UNIX, "resolveIndirectDataBlocks - quit search for indirect blocks\n");
      break;
    }

    debug(FS_UNIX, "resolveIndirectDataBlocks - new_sector=%x\n", new_sector);

    // single-indirect block means that the gained (resolved) sector-address
    // points to a data-block of the i_node
    if(degree_of_indirection == 1)
    {
      inode->addSector(new_sector);
    }
    else
    {
      resolveIndirectDataBlocks(inode, new_sector, degree_of_indirection-1, sector_addr_len);
    }
  }

  volume_manager->releaseReadDataBlock(indirect_block);
  delete[] sector;
}

sector_addr_t FileSystemUnix::storeIndirectDataBlocks(Inode* inode, sector_addr_t inode_sector_to_store,
                                      sector_addr_t indirect_block, uint16 degree_of_indirection,
                                      uint16 sector_addr_len)
{
  assert(inode != NULL);

  if(degree_of_indirection == 0)
  {
    debug(FS_UNIX, "storeIndirectDataBlocks - sorry this method only works for indirect blocks - quit\n");
    return indirect_block;
  }

  debug(FS_UNIX, "storeIndirectDataBlocks - CALL InodeID=%d store %dth sector\n", inode->getID(), inode_sector_to_store);

  // the clean-up flag is set as soon as the last sector to store
  // was reached, all subsequent sectors will be freed!
  bool cleanup_following_sectors = false;
  bool block_is_empty = false;

  if(inode->getNumSectors() <= inode_sector_to_store)
  {
    // nothing to store, no indirect sector available so nothing to do here
    if(indirect_block == UNUSED_DATA_BLOCK)
    {
      return UNUSED_DATA_BLOCK;
    }

    cleanup_following_sectors = true;
    block_is_empty = true;
  }

  debug(FS_UNIX, "storeIndirectDataBlocks - indirect_block %x deg_of_indirection=%d\n", indirect_block, degree_of_indirection);
  FileSystemUnix* fs = static_cast<FileSystemUnix*>(inode->getFileSystem());
  assert(fs != NULL);
  FsVolumeManager* volume_manager = fs->getVolumeManager();
  assert(volume_manager != NULL);

  // no indirect-block given, so request a new in order to be able to store
  // the Inode's data blocks
  if(indirect_block == UNUSED_DATA_BLOCK)
  {
    // request a new block of data
    indirect_block = fs->occupyAndReturnFreeBlock(true);
    if(indirect_block == 0x0)
    {
      debug(FS_UNIX, "storeIndirectDataBlocks - ERROR failed to get a free data-block!\n");
      return -1U;
    }
  }

  // lock sector for writing
  volume_manager->acquireDataBlockForWriting(indirect_block);

  // readout sector data
  char* sector = volume_manager->readDataBlockUnprotected(indirect_block);

  if(sector == NULL)
  {
    debug(FS_UNIX, "storeIndirectDataBlocks - ERROR failed to read block %x!\n", indirect_block);

    volume_manager->releaseWriteDataBlock(indirect_block);
    return -1U;
  }

  // scan the current block for more indirect blocks
  for(sector_len_t i = 0; i < fs->getDataBlockSize(); i+= sector_addr_len)
  {
    sector_addr_t current_entry = 0;

    // reading the current sector entry in a very flexible way
    for(uint16 j = 0; j < sector_addr_len; j++)
    {
      //debug(FS_UNIX, "storeIndirectDataBlocks - sector[%d + %d]=%x\n", i, j, (uint8)sector[i + j]);
      current_entry |= ((uint8)(sector[i + j])) << (8*j);
    }

    // clean-up mode - remove all sectors that appear now!
    if(cleanup_following_sectors)
    {
      // no more data-blocks are stored here, quit!
      if(current_entry == UNUSED_DATA_BLOCK)
      {
        debug(FS_UNIX, "storeIndirectDataBlocks - no more data-blocks; stop with clean-up. DONE\n");
        break;
      }

      // free the remaining data-blocks recursively, note: that the Inode's sectors
      // where already released
      if(degree_of_indirection > 1)
      {
        // perform a recursive clean-up
        sector_addr_t result = storeIndirectDataBlocks(inode, inode_sector_to_store,
                                                       current_entry,
                                                       degree_of_indirection-1,
                                                       sector_addr_len);

        if(result != UNUSED_DATA_BLOCK)
        {
          debug(FS_UNIX, "storeIndirectDataBlocks - ERROR failed to clean %x!\n", current_entry);
        }
      }

      // mark the current entry as free:
      memset(sector + i, 0x00, sector_addr_len);
    }
    // store the rest of the Inode's data blocks
    else
    {
      // now we store the sectors directly
      if(degree_of_indirection == 1)
      {
        sector_addr_t inode_cur_sector = inode->getSector(inode_sector_to_store);
        assert(inode_cur_sector != 0); // should not get to 0 here!

        memcpy(sector + i, reinterpret_cast<char*>(&inode_cur_sector), sector_addr_len);

        inode_sector_to_store++;
        if(inode_sector_to_store >= inode->getNumSectors())
        {
          // no more sectors, now operate in clean-up mode
          cleanup_following_sectors = true;
        }
      }
      // degree > 1
      else
      {
        // no data block available, request one!
        if(current_entry == UNUSED_DATA_BLOCK)
        {
          // request a new data-block
          current_entry = fs->occupyAndReturnFreeBlock(true);

          if(current_entry == 0x0)
          {
            debug(FS_UNIX, "storeIndirectDataBlocks - ERROR failed to get a free data-block!\n");
            break;
          }

          // store reference to new data-block
          memcpy(sector + i, reinterpret_cast<char*>(&current_entry), sector_addr_len);

        }

        // perform a recursive call, Inode is still the same, use current-sector
        // counter event if it is overflow, decrease the degree of indirection by
        // one and the sector_addr_len stays the same
        storeIndirectDataBlocks(inode, inode_sector_to_store, current_entry, degree_of_indirection-1, sector_addr_len);

        // increase the current-sector by the number of sectors that could
        // be stored in the chain of indirection
        // num_entries_per_data_block ^ (deg-1) = num_data_blocks_addressed
        inode_sector_to_store += pow( fs->getDataBlockSize()/sector_addr_len, degree_of_indirection-1 );

        if(inode_sector_to_store >= inode->getNumSectors())
        {
          // no more sectors, now operate in clean-up mode
          cleanup_following_sectors = true;
        }

      }

    }

  }

  // rewrite the data-block
  volume_manager->writeDataBlockUnprotected(indirect_block, sector);

  // leave mutual exclusion
  volume_manager->releaseWriteDataBlock(indirect_block);

  delete[] sector;

  // indirect-block has no more references and is therefore unused ...
  if(block_is_empty)
  {
    // ... so free the block
    if(!fs->freeOccupiedBlock(indirect_block))
    {
      debug(FS_UNIX, "storeIndirectDataBlocks - ERROR failed to free %x!\n", indirect_block);
    }

    indirect_block = UNUSED_DATA_BLOCK;
  }

  return indirect_block;
}

char* FileSystemUnix::safeEscapeFilename(const char* filename, uint32 filename_len)
{
  bool string_escaped = false;

  for(uint32 i = 0; i < filename_len; i++)
  {
    // string seems to be escaped:
    if(filename[i] == '\0')
    {
      string_escaped = true;
      break;
    }
  }

  if(string_escaped)
  {
    return strdup(filename);
  }

  // string is not escaped; use the maximum filename length + 1 char for the '\0'
  char* str = new char[filename_len + 1];
  strncpy(str, filename, filename_len);
  str[filename_len] = '\0';

  return str;
}
