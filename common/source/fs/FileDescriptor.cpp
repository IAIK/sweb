#include "FileDescriptor.h"
#include <ulist.h>
#include "ArchThreads.h"
#include "Mutex.h"
#include "kprintf.h"

ustl::list<FileDescriptor*> global_fd;
Mutex global_fd_lock("global_fd_lock");

static uint32 fd_num_ = 3;

void FileDescriptor::add(FileDescriptor* fd)
{
  global_fd_lock.acquire();
  global_fd.push_back(fd);
  global_fd_lock.release();
}

void FileDescriptor::remove(FileDescriptor* fd)
{
  global_fd_lock.acquire();
  global_fd.remove(fd);
  global_fd_lock.release();
}

FileDescriptor::FileDescriptor(File* file)
{
  fd_ = ArchThreads::atomic_add(fd_num_, 1);
  file_ = file;
}
