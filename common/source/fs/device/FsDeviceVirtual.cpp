/**
 * Filename: FsDeviceVirtual.cpp
 * Description:
 *
 * Created on: 19.05.2012
 * Author: chris
 */

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS

#include "fs/device/FsDeviceVirtual.h"
#include "arch_bd_virtual_device.h"

#include "kprintf.h"

FsDeviceVirtual::FsDeviceVirtual(BDVirtualDevice* device) : dev_(device)
{
  assert(dev_ != NULL);
}

FsDeviceVirtual::~FsDeviceVirtual()
{
}

bool FsDeviceVirtual::readSector(sector_addr_t sector, char* buffer, sector_len_t buffer_size)
{
  debug(FS_DEVICE, "readSector - CALL\n");

  if(buffer_size % getBlockSize() != 0)
    return false;

  debug(FS_DEVICE, "writeSector - reading sector=%x buffer=%x (len=%d)\n", sector, buffer, buffer_size);

  // reading data
  if(dev_->readData(sector * getBlockSize(), buffer_size, buffer) == -1)
    return false;

  return true;
}

bool FsDeviceVirtual::writeSector(sector_addr_t sector, const char* buffer, sector_len_t buffer_size)
{
  debug(FS_DEVICE, "writeSector - CALL\n");

  if(buffer_size % getBlockSize() != 0)
    return false;

  // some debug-statements:
  debug(FS_DEVICE, "writeSector - writing sector=%x buffer=%x (len=%d)\n", sector, buffer, buffer_size);

  //for(sector_len_t i = 0; i < buffer_size; i++)
  //{
    //if((i+1) % 32 == 0) kprintfd("%x(%c)\n", 0x000000FF & (uint32)(buffer[i]), buffer[i]);
    //else kprintfd("%x(%c) ", buffer[i] & 0x00FF, buffer[i]);
  //}

  //kprintfd("\n");
  //debug(FS_DEVICE, "print complete...\n");

  // duplicate data-block:
  char* buf_cpy = new char[buffer_size];
  memcpy(buf_cpy, buffer, buffer_size);

  // writing data:
  if(dev_->writeData(sector * getBlockSize(), buffer_size, buf_cpy ) == -1)
  {
    delete[] buf_cpy;
    return false;
  }

  delete[] buf_cpy;
  return true;
}

void FsDeviceVirtual::setBlockSize(sector_len_t new_block_size)
{
  if(new_block_size % 512 != 0)
    return;

  dev_->setBlockSize(new_block_size);
}

sector_len_t FsDeviceVirtual::getBlockSize() const
{
  return dev_->getBlockSize();
}

sector_addr_t FsDeviceVirtual::getNumBlocks(void) const
{
  return dev_->getNumBlocks();
}

#endif // USE_FILE_SYSTEM_ON_GUEST_OS
