/**
 * Filename: FileBlockDevice.cpp
 * Description:
 *
 * Created on: 06.06.2012
 * Author: chris
 */

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS

#include "fs/FileDescriptor.h"
#include "fs/inodes/FileBlockDevice.h"
#include "BDVirtualDevice.h"

FileBlockDevice::FileBlockDevice(BDVirtualDevice* block_dev) : File(0,0,0,NULL,0,0,0,0,0), block_dev_(block_dev)
{
}

FileBlockDevice::~FileBlockDevice()
{
}

Inode::InodeType FileBlockDevice::getType(void) const
{
  return Inode::InodeTypeBlockDevice;
}

BDVirtualDevice* FileBlockDevice::getDevice(void)
{
  return block_dev_;
}

const BDVirtualDevice* FileBlockDevice::getDevice(void) const
{
  return block_dev_;
}

int32 FileBlockDevice::read(FileDescriptor* fd, char* buffer, uint32 len)
{
  block_dev_->readData(fd->getCursorPos(), len, buffer);
  return 0;
}

int32 FileBlockDevice::write(FileDescriptor* fd, const char* buffer, uint32 len)
{
  block_dev_->writeData(fd->getCursorPos(), len, strdup( buffer ));
  return 0;
}

#endif // USE_FILE_SYSTEM_ON_GUEST_OS
