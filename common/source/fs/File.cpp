// Projectname: SWEB
// Simple operating system for educational purposes

#include "fs/File.h"

File::File(Inode* inode, Dentry* dentry, uint32 flag)
{
  f_inode_ = inode;
  f_dentry_ = dentry;
  flag_ = flag;
}
