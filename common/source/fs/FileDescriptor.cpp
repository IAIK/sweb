#include "FileDescriptor.h"
#include <ulist.h>
#ifndef EXE2MINIXFS
#include "ArchThreads.h"
#include "Mutex.h"
#endif
#include "kprintf.h"

ustl::list<FileDescriptor*> global_fd;
Mutex global_fd_lock("global_fd_lock");

static size_t fd_num_ = 3;

void FileDescriptor::add(FileDescriptor* fd)
{
  MutexLock ml(global_fd_lock);
  global_fd.push_back(fd);
}

void FileDescriptor::remove(FileDescriptor* fd)
{
  MutexLock ml(global_fd_lock);
  global_fd.remove(fd);
}

FileDescriptor::FileDescriptor(File* file)
{
  fd_ = ArchThreads::atomic_add(fd_num_, 1);
  file_ = file;
}
