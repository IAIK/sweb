/**
 * Filename: FileSystemPool.cpp
 * Description:
 *
 * Created on: 12.07.2012
 * Author: chris
 */

#include "fs/device/FsDevice.h"
#include "fs/FileSystem.h"
#include "fs/FileSystemPool.h"

#include "fs/FileSystemInfo.h"
#include "fs/minix/MinixFileSystemInfo.h"

#ifdef USE_FILE_SYSTEM_ON_GUEST_OS
#include <cstring>
#else
#include "util/string.h"
#endif

FileSystemPool::FileSystemPool()
{
  supported_file_systems_.push_back(new MinixFileSystemInfo());

  // TODO add the Info object of your FS implementation here
}

FileSystemPool::~FileSystemPool()
{
  // delete all FS-Info objects
  for(uint32 i = 0; i < supported_file_systems_.size(); i++)
  {
    delete supported_file_systems_[i];
  }
}

FileSystem* FileSystemPool::getNewFsInstance(FsDevice* device, uint8 part_ident, uint32 mnt_flags)
{
  for(uint32 i = 0; i < supported_file_systems_.size(); i++)
  {
    if(supported_file_systems_[i]->getPartitionIdent() == part_ident)
    {
      return supported_file_systems_[i]->getNewFileSystemInstance(device, mnt_flags);
    }
  }

  // nothing suitable found...
  return NULL;
}

FileSystem* FileSystemPool::getNewFsInstance(FsDevice* device, const char* fs_name, uint32 mnt_flags)
{
  if(fs_name == NULL)
    return NULL;

  for(uint32 i = 0; i < supported_file_systems_.size(); i++)
  {
    if( strncmp(supported_file_systems_[i]->getName(), fs_name, strlen(fs_name)) == 0 )
    {
      return supported_file_systems_[i]->getNewFileSystemInstance(device, mnt_flags);
    }
  }

  // nothing suitable found...
  return NULL;
}
