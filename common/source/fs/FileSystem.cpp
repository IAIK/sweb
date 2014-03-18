/**
 * Filename: FileSystem.cpp
 * Description:
 *
 * Created on: 13.05.2012
 * Author: chris
 */

#include "fs/VfsSyscall.h"
#include "fs/FileSystem.h"
#include "fs/FsVolumeManager.h"

#include "fs/device/FsDevice.h"

#include "fs/inodes/Directory.h"

#include "cache/GeneralCache.h"
#include "cache/CacheFactory.h"
#include "cache/FiFoReadNoWriteCacheFactory.h"
#include "cache/FifoReadWriteBackCacheFactory.h"
#include "fs/DeviceCache.h"

#include "fs/FsSmartLock.h"
#include "fs/FsDefinitions.h"

#ifdef USE_FILE_SYSTEM_ON_GUEST_OS
#include <cstring>
#endif

FileSystem::FileSystem(FsDevice* device, uint32 mount_flags) : device_(device),
    volume_manager_(NULL), root_(NULL), inode_cache_(NULL),
    mount_flags_(mount_flags)
{
  // TODO implement an algorithm for selecting an appropriate
  // Sector-cache size:
  sector_addr_t num_cached_blocks = 256; //device_->getNumBlocks() / 100;
  //if(num_cached_blocks < 10) num_cached_blocks = 10;
  // sector cache should never(!!) exceed available kernel-memory, because
  // swapping of Cache memory, would be senseless

  Cache::CacheFactory* fs_cache_factory = NULL;

  // the mount-flag MS_SYNCHRONOUS makes all write-operations to the FileSystem
  // Synchronous which means that they are executed immediately
  if( mount_flags & MS_SYNCHRONOUS )
  {
    fs_cache_factory = new Cache::FiFoReadNoWriteCacheFactory();
    //fs_cache_factory = new Cache::AgingReadNoWriteCacheFactory();
  }
  // in all other cases use the Write-Back strategy
  else
  {
    fs_cache_factory = new Cache::FifoReadWriteBackCacheFactory();
    //fs_cache_factory = new Cache::AgingReadWriteBackCacheFactory();
  }

  // creating the VolumeManager
  volume_manager_ = new FsVolumeManager(this, device_, fs_cache_factory, num_cached_blocks);

  // TODO implement an algorithm for selecting an appropriate number
  // of cached items
  Cache::num_items_t max_cache_items = 512;

  // creating the Inode-Cache
  inode_cache_ = fs_cache_factory->getNewCache(NULL, max_cache_items);

  delete fs_cache_factory;
}

FileSystem::~FileSystem()
{
  debug(FILE_SYSTEM, "~FileSystem() - CALL destroying FS\n");

  // first destroy and flush the InodeCache
  if(inode_cache_ != NULL)
  {
    delete inode_cache_;
    debug(FILE_SYSTEM, "~FileSystem() - deleted the Inode Cache\n");
  }

  // finally destroy the VolumeManager
  if(volume_manager_ != NULL)
  {
    delete volume_manager_;
    debug(FILE_SYSTEM, "~FileSystem() - deleted the Volume Manager\n");
  }

  if(device_ != NULL)
  {
    delete device_;
  }
}

uint32 FileSystem::getMountFlags(void) const
{
  return mount_flags_;
}

Directory* FileSystem::getRoot(void)
{
  debug(FILE_SYSTEM, "getRoot() - CALL returning the root inode\n");
  assert(this->acquireInode(root_->getID(), NULL, root_->getName()) == root_);
  return root_;
}

//const Directory* FileSystem::getRoot(void) const
//{
  //return getRoot();
//}

sector_len_t FileSystem::getBlockSize(void) const
{
  return device_->getBlockSize();
}

sector_len_t FileSystem::getDataBlockSize(void) const
{
  return getBlockSize();
}

FsVolumeManager* FileSystem::getVolumeManager(void)
{
  return volume_manager_;
}

bool FileSystem::isFilenameValid(const char* filename, uint32 str_len)
{
  if(strlen(filename) != str_len)
  {
    // '\0'-chars are not allowed in a filename!
    return false;
  }

  // is there a Separator-char in the filename?
  for(uint32 i = 0; i < str_len; i++)
  {
    if(filename[i] == VfsSyscall::getLastDefinedPathSeparator())
    {
      // the path-separator of a Vfs is never a legal-char for filenames!
      return false;
    }
  }

  // all checks passed, the file is valid
  return true;
}

Inode* FileSystem::lookup(Directory* cur_inode, const char* inode_name)
{
  assert(cur_inode != NULL);
  debug(FILE_SYSTEM, "lookup - CALL looking for \"%s\" in parent ID=%d\n", inode_name, cur_inode->getID());

  FileSystemLock* inode_lock = cur_inode->getLock();

  // acquire the I-Node data for reading for the moment
  inode_lock->acquireReadBlocking();

  if(cur_inode->areChildrenLoaded())
  {
    debug(FILE_SYSTEM, "lookup - children are already loaded.\n");

    // all children are loaded, search for an I-Node child with the
    // given filename
    Inode* inode = cur_inode->getInode(inode_name);
    inode_lock->releaseRead();

    return inode;
  }

  // children were not loaded so far, so load now
  inode_lock->releaseRead();

  // acquire the I-Node for writing data to it!
  FsWriteSmartLock inode_write_lock(inode_lock);

  // maybe some-one else just loaded the Children into the cache?
  if(cur_inode->areChildrenLoaded())
  {
    // just search the I_Node
    return cur_inode->getInode(inode_name);
  }

  // now still old state, we have to load the children to the cache!
  debug(FILE_SYSTEM, "lookup - going to read Directory's contents from the FS.\n");
  loadDirChildrenUnsafe(cur_inode);

  return cur_inode->getInode(inode_name);
}

void FileSystem::sync(void)
{
  // flushing the i-node cache ...
  if(inode_cache_ != NULL)
  {
    inode_cache_->flush();
  }

  // ... and the device cache
  if(volume_manager_ != NULL)
  {
    volume_manager_->flush();
  }
}

int32 FileSystem::fsync(Inode* inode)
{
  int32 ret_val = -1;

  // synchronize the I-Node's data-blocks that are pending
  if(volume_manager_ != NULL)
  {
    for(sector_addr_t i = 0; inode->getSector(i) != 0; i++)
    {
      if(volume_manager_->synchronizeSector( inode->getSector(i) ))
      {
        ret_val = 0;
      }
    }
  }

  return ret_val;
}
