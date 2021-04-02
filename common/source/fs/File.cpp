#include "File.h"
#include "Inode.h"
#include "FileDescriptor.h"
#include "assert.h"


File::File(Inode* inode, Dentry* dentry, uint32 flag) :
    uid(0), gid(0), version(0), f_superblock_(0), f_inode_(inode), f_dentry_(dentry), flag_(flag)
{
}

File::~File()
{
  debug(VFS_FILE, "Destroying file\n");

  if(!f_fds_.empty())
  {
    debug(VFS_FILE, "WARNING: File still has open file descriptors\n");
  }

  for(FileDescriptor* fd : f_fds_)
  {
    fd->file_ = nullptr;
    delete fd;
  }
}

uint32 File::getSize()
{
  return f_inode_->getSize();
}

l_off_t File::lseek(l_off_t offset, uint8 origin)
{
  if (origin == SEEK_SET)
    offset_ = offset;
  else if (origin == SEEK_CUR)
    offset_ += offset;
  else if (origin == SEEK_END)
    offset_ = f_inode_->getSize() + offset;
  else
    return -1;

  return offset_;
}




FileDescriptor* File::openFd()
{
    debug(VFS_FILE, "Open new file descriptor\n");
    FileDescriptor* fd = new FileDescriptor(this);
    f_fds_.push_back(fd);

    debug(VFS_FILE, "New file descriptor num: %u\n", fd->getFd());
    return fd;
}


int File::closeFd(FileDescriptor* fd)
{
    debug(VFS_FILE, "Close file descriptor num %u\n", fd->getFd());
    assert(fd);

    f_fds_.remove(fd);
    delete fd;

    // Release on last fd removed
    if(f_fds_.empty())
    {
        debug(VFS_FILE, "Releasing file\n");
        return getInode()->release(this); // Will delete this file object
    }

    return 0;
}
