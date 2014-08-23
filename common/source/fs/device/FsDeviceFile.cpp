/**
 * Filename: FsDeviceFile.cpp
 * Description:
 *
 * Created on: 13.05.2012
 * Author: chris
 */

#ifdef USE_FILE_SYSTEM_ON_GUEST_OS

#include "fs/device/FsDeviceFile.h"

#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

FsDeviceFile::FsDeviceFile(const char* image_file, sector_addr_t offset,
    sector_addr_t part_size, sector_len_t block_size)
    : img_fd_(0), /*image_file_(NULL),*/ partition_offset_(offset), partition_len_(part_size),
      block_size_(block_size), num_blocks_(0), image_size_(0)
{
  // open the image file-for binary reading
  img_fd_ = open(image_file, O_RDWR | O_BINARY);
  assert(img_fd_ > 0);
  //image_file_ = fopen(image_file, "rb");

  // determining the size of the image:
  image_size_ = lseek(img_fd_, 0, SEEK_END);
  lseek(img_fd_, 0, SEEK_SET);

  //fseek(image_file_, 0, SEEK_END);
  //image_size_ = ftell(image_file_);
  //fseek(image_file_, 0, SEEK_SET);

  assert(partition_offset_ < image_size_);
  assert(partition_offset_ + partition_len_ <= image_size_);
}

FsDeviceFile::~FsDeviceFile()
{
  if(img_fd_ > 0)
  {
    close(img_fd_);
  }

  /*if(image_file_ != NULL)
  {
    // closing the image, we are done!
    fclose(image_file_);
  }*/
}

bool FsDeviceFile::readSector(sector_addr_t sector, char* buffer, sector_len_t buffer_size)
{
  assert(buffer_size % block_size_ == 0);

  off_t offset = lseek(img_fd_, partition_offset_ + sector * getBlockSize(), SEEK_SET);
  assert(offset + buffer_size <= partition_offset_ + partition_len_);

  // reading contents from the image-file
  ssize_t read_bytes = ::read(img_fd_, buffer, buffer_size);

  if(read_bytes == buffer_size)
    return true;

  return false;
}

bool FsDeviceFile::writeSector(sector_addr_t sector, const char* buffer, sector_len_t buffer_size)
{
  assert(buffer_size % block_size_ == 0);

  off_t offset = lseek(img_fd_, partition_offset_ + sector * getBlockSize(), SEEK_SET);
  assert(offset + buffer_size <= partition_offset_ + partition_len_);

  // writing to the image, overriding old contents at this position
  ssize_t written_bytes = ::write(img_fd_, buffer, buffer_size);

  if(written_bytes == buffer_size)
    return true;

  return false;
}

void FsDeviceFile::setBlockSize(sector_len_t new_block_size)
{
  if(new_block_size % SECTOR_SIZE != 0)
    return;

  block_size_ = new_block_size;

  num_blocks_ = partition_len_ / block_size_;
}

sector_len_t FsDeviceFile::getBlockSize(void) const
{
  return block_size_;
}

sector_addr_t FsDeviceFile::getNumBlocks(void) const
{
  return num_blocks_;
}

#endif // USE_FILE_SYSTEM_ON_GUEST_OS
