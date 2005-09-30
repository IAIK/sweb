// Projectname: SWEB
// Simple operating system for educational purposes

#include "fs/Superblock.h"
#include "assert.h"
#include "Dentry.h"
#include "Inode.h"
#include "File.h"

//------------------------------------------------------------------
Superblock::~Superblock()
{
}
//
//------------------------------------------------------------------
void Superblock::delete_inode(Inode *inode)
{
  assert(inode != 0);
  int32 del_inode = dirty_inodes_.remove(inode);
  if(del_inode == -1)
    del_inode = used_inodes_.remove(inode);
  assert(del_inode != -1);
  delete inode;
}

//------------------------------------------------------------------
Dentry *Superblock::getRoot()
{
  return s_root_;
}

//------------------------------------------------------------------
Dentry *Superblock::getMountPoint()
{
  return mounted_over_;
}
