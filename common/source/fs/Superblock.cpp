#include "Superblock.h"
#include "assert.h"
#include "Dentry.h"
#include "Inode.h"
#include "File.h"

Superblock::Superblock(Dentry* s_root, uint32 s_dev)
{
  s_root_ = s_root;
  s_dev_ = s_dev;
}

Superblock::~Superblock()
{
}

void Superblock::delete_inode(Inode* inode)
{
  assert(inode != 0);
  dirty_inodes_.remove(inode);
  used_inodes_.remove(inode);
  all_inodes_.remove(inode);
  delete inode;
}

Dentry* Superblock::getRoot()
{
  return s_root_;
}

Dentry* Superblock::getMountPoint()
{
  return mounted_over_;
}

FileSystemType* Superblock::getFSType()
{
  return (FileSystemType*) s_type_;
}
