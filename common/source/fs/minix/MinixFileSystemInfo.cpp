/**
 * Filename: MinixFileSystemInfo.cpp
 * Description:
 *
 * Created on: 12.07.2012
 * Author: chris
 */

#include "fs/minix/MinixFileSystemInfo.h"

#include "fs/device/FsDevice.h"
#include "fs/FileSystem.h"
#include "fs/minix/FileSystemMinix.h"
#include "fs/minix/MinixDefs.h"

MinixFileSystemInfo::MinixFileSystemInfo()
{
}

MinixFileSystemInfo::~MinixFileSystemInfo()
{
}

uint8 MinixFileSystemInfo::getPartitionIdent(void) const
{
  return 0x81;
}

const char* MinixFileSystemInfo::getName(void) const
{
  return "minix";
}

FileSystem* MinixFileSystemInfo::getNewFileSystemInstance(FsDevice* device, uint32 mount_flags)
{
  // the superblock for the Minix:
  minix_super_block sb;

  device->setBlockSize(MINIX_BLOCK_SIZE);

  // 1. read out the super-block of the fs
  if( !FileSystemMinix::readSuperblock(device, sb) )
  {
    // invalid Superblock, not a minix!
    return NULL;
  }

  // 2. decide which version of minix will be created
  uint16 filename_len = 30;

  if( sb.s_magic == MINIX_SUPER_MAGIC || sb.s_magic == MINIX2_SUPER_MAGIC )
  {
    filename_len = 14;
  }

  // 3 detect version to mount
  uint16 version = 1;

  if( sb.s_magic == MINIX2_SUPER_MAGIC || sb.s_magic == MINIX2_SUPER_MAGIC2 )
  {
    // use MinixV2
    version = 2;
  }

  // 4. return the instance
  return new FileSystemMinix(device, mount_flags, sb, version, filename_len);
}
