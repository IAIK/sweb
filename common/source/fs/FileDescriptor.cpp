/**
 * @file FileDescriptor.cpp
 */

#include "fs/FileDescriptor.h"
#include "fs/FileSystem.h"

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
#include <ustl/ulist.h>
#include "ArchThreads.h"

#include "Mutex.h"
#include "MutexLock.h"

#define STL_NAMESPACE_PREFIX      ustl::
#else
#include <list>
#define STL_NAMESPACE_PREFIX      std::
#endif

STL_NAMESPACE_PREFIX list<FileDescriptor*> global_fd;

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
Mutex global_fd_lock("global_fd_lock");
#endif

static uint32 fd_num_ = 3;

void FileDescriptor::add(FileDescriptor* fd)
{
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  global_fd_lock.acquire();
#endif

  global_fd.push_back(fd);

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  global_fd_lock.release();
#endif
}

void FileDescriptor::remove(FileDescriptor* fd)
{
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  global_fd_lock.acquire();
#endif

  global_fd.remove(fd);
  delete fd;

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  global_fd_lock.release();
#endif
}

bool FileDescriptor::remove(fd_size_t fd)
{
  bool remove_sucessfully = false;

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  global_fd_lock.acquire();
#endif

  for(STL_NAMESPACE_PREFIX list<FileDescriptor*>::iterator it = global_fd.begin(); it != global_fd.end(); it++)
  {
    if(*it && (*it)->getFd() == fd)
    {
      delete (*it);
      global_fd.erase(it);

      debug(VFSSYSCALL, "close() - OK - removed FD from the list\n");
      remove_sucessfully = true;
      break;
    }
  }

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  global_fd_lock.release();
#endif
  return remove_sucessfully;
}

FileDescriptor* FileDescriptor::getFileDescriptor(fd_size_t fd)
{
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  extern Mutex global_fd_lock;
#endif

  FileDescriptor* file_descriptor = 0;

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  MutexLock mlock(global_fd_lock);
#endif

  for (STL_NAMESPACE_PREFIX list<FileDescriptor*>::iterator it = global_fd.begin(); it != global_fd.end(); it++)
  {
    if ((*it)->getFd() == fd)
    {
      file_descriptor = *it;
      debug(VFSSYSCALL, "found the fd!\n");
      break;
    }
  }
  return file_descriptor;
}

FileDescriptor::FileDescriptor ( File* file, bool append_mode,
                                 bool nonblocking_mode ) : file_(file), cursor_pos_(0),
                                 append_mode_(append_mode),
                                 nonblocking_mode_(nonblocking_mode), read_mode_(false),
                                 write_mode_(true), synchronous_(false)
{
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  fd_ = ArchThreads::atomic_add(fd_num_, 1);
#else
  fd_ = fd_num_++;
#endif
  //file_ = file;
}

FileDescriptor::FileDescriptor(const FileDescriptor& cpy) : fd_(cpy.fd_),
    file_(cpy.file_), cursor_pos_(cpy.cursor_pos_), owner_(cpy.owner_),
    append_mode_(cpy.append_mode_), nonblocking_mode_(cpy.nonblocking_mode_),
    read_mode_(cpy.read_mode_), write_mode_(cpy.write_mode_),
    synchronous_(cpy.synchronous_)
{
  // IMPORTANT: the fs uses a reference counter to prevent from deleting an object
  // instance while it is still in use; now that file_ is used once more we need
  // to manually increment the reference counter of it
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  assert ( file_->getFileSystem()->acquireInode(file_->getID(), 0, file_->getName()) == file_ );
#endif
}

FileDescriptor::~FileDescriptor()
{
  debug(VFSSYSCALL, "~FileDescriptor() - fd destructor\n");

  if(file_ != NULL)
  {
    // release the File-node from the FileSystem
    FileSystem* fs = file_->getFileSystem();
    fs->releaseInode(file_);
    debug(VFSSYSCALL, "~FileDescriptor() - released File (Inode)\n");
  }
}

void FileDescriptor::setReadMode(bool mode)
{
  read_mode_ = mode;
}

void FileDescriptor::setWriteMode(bool mode)
{
  write_mode_ = mode;
}

bool FileDescriptor::readMode(void) const
{
  return read_mode_;
}

bool FileDescriptor::writeMode(void) const
{
  return write_mode_;
}

void FileDescriptor::setSynchroniousWrite(bool mode)
{
  synchronous_ = mode;
}

bool FileDescriptor::synchronizeMode(void) const
{
  return synchronous_;
}

file_size_t FileDescriptor::getCursorPos(void) const
{
  return cursor_pos_;
}

file_size_t FileDescriptor::setCursorPos(file_size_t new_pos)
{
  return cursor_pos_ = new_pos;
}

file_size_t FileDescriptor::moveCursor(file_size_t movement, bool to_the_end)
{
  if(to_the_end)
  {
    cursor_pos_ += movement;
    // file-cursor can be set beyond the file-size (see POSIX lseek())
    //if(cursor_pos_ >= file_->getFileSize()) cursor_pos_ = file_->getFileSize()-1;
  }
  else
  {
    // mind the underflow!
    if(movement > cursor_pos_) cursor_pos_ = 0;
    else cursor_pos_ -= movement;
  }
  return cursor_pos_;
}

bool FileDescriptor::appendMode(void) const
{
  return append_mode_;
}

bool FileDescriptor::nonblockingMode(void) const
{
  return nonblocking_mode_;
}
