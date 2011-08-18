/**
 * @file FileDescriptor.cpp
 */

#include "fs/FileDescriptor.h"
#include <ustl/ulist.h>
#include "ArchThreads.h"

ustl::list<FileDescriptor*> global_fd;

static uint32 fd_num_ = 3;

FileDescriptor::FileDescriptor(File* file)
{
  fd_ = ArchThreads::atomic_add(fd_num_, 1);
  file_ = file;
}
