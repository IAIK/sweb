/**
 * @file FileDescriptor.cpp
 */

#include "fs/FileDescriptor.h"
#include "fs/PointList.h"

PointList<FileDescriptor> global_fd;

static uint32 fd_num_ = 3;

FileDescriptor::FileDescriptor(File* file)
{
  fd_ = fd_num_;
  fd_num_++;
  file_ = file;
}
