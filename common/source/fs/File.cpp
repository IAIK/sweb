#include "File.h"
#include "Inode.h"
#include "FileDescriptor.h"
#include "assert.h"
#include "Superblock.h"
#include "FileSystemType.h"


File::File(Inode* inode, Dentry* dentry, uint32 flag) :
    uid(0),
    gid(0),
    version(0),
    f_superblock_(inode->getSuperblock()),
    f_inode_(inode),
    f_dentry_(dentry),
    flag_(flag),
    offset_(0)
{
    f_inode_->incRefCount();
}

File::~File()
{
  debug(VFS_FILE, "Destroying file\n");

  assert(f_fds_.empty() && "File to be destroyed still has open file descriptors");
  assert(f_inode_);

  if(!f_inode_->decRefCount())
  {
      debug(VFS_FILE, "No more references to %s inode %p left after closing file, deleting\n",
            f_inode_->getSuperblock()->getFSType()->getFSName(), f_inode_);
    f_inode_->getSuperblock()->deleteInode(f_inode_);
  }
}

uint32 File::getSize()
{
  return f_inode_->getSize();
}

l_off_t File::lseek(l_off_t offset, uint8 origin)
{
  debug(VFS_FILE, "(lseek) offset: %llu, origin: %u\n", (long long unsigned int)offset, origin);
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


SimpleFile::SimpleFile(Inode* inode, Dentry* dentry, uint32 flag) :
    File(inode, dentry, flag)
{
}

int32 SimpleFile::read(char *buffer, size_t count, l_off_t offset)
{
    debug(VFS_FILE, "(read) buffer: %p, count: %zu, offset: %llu(%zu)\n", buffer, count, (long long unsigned int)offset, (size_t)(offset_ + offset));
    if (((flag_ & O_RDONLY) || (flag_ & O_RDWR)) && (f_inode_->getMode() & A_READABLE))
    {
        int32 read_bytes = f_inode_->readData(offset_ + offset, count, buffer);
        offset_ += read_bytes;
        return read_bytes;
    }
    else
    {
        // ERROR_FF
        return -1;
    }
}

int32 SimpleFile::write(const char *buffer, size_t count, l_off_t offset)
{
    debug(VFS_FILE, "(write) buffer: %p, count: %zu, offset: %llu(%zu)\n", buffer, count, (long long unsigned int)offset, (size_t)(offset_ + offset));
    if (((flag_ & O_WRONLY) || (flag_ & O_RDWR)) && (f_inode_->getMode() & A_WRITABLE))
    {
        int32 written_bytes = f_inode_->writeData(offset_ + offset, count, buffer);
        offset_ += written_bytes;
        return written_bytes;
    }
    else
    {
        // ERROR_FF
        return -1;
    }
}
