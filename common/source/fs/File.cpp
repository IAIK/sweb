#include "File.h"
#include "Inode.h"

File::File(Inode* inode, Dentry* dentry, uint32 flag) :
    uid(0), gid(0), version(0), f_superblock_(0), f_inode_(inode), f_dentry_(dentry), flag_(flag)
{
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
