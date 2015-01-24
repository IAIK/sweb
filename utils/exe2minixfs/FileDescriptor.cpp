/**
 * @file FileDescriptor.cpp
 */

#include "FileDescriptor.h"
#include "PointList.h"
//#include "ArchThreads.h"

PointList<FileDescriptor> global_fd;

static uint32 fd_num_ = 3;

FileDescriptor::FileDescriptor(File* file)
{
  fd_ = fd_num_++;
  file_ = file;
}
