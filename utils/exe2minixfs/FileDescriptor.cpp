/**
 * @file FileDescriptor.cpp
 */

#include "FileDescriptor.h"
#include <list>

std::list<FileDescriptor*> global_fd;


static uint32 fd_num_ = 3;

void FileDescriptor::add(FileDescriptor* fd)
{
  global_fd.push_back(fd);
}

void FileDescriptor::remove(FileDescriptor* fd)
{
  global_fd.remove(fd);
}

FileDescriptor::FileDescriptor(File* file)
{
  fd_ = fd_num_++;
  file_ = file;
}
