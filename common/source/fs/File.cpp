/**
 * @file File.cpp
 */

#include "fs/File.h"
#include "Inode.h"

File::File ( Inode* inode, Dentry* dentry, uint32 flag )
{
  f_inode_ = inode;
  f_dentry_ = dentry;
  flag_ = flag;
}


uint32 File::getSize()
{
  return f_inode_->getSize();
}
