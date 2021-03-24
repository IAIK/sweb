#include "Superblock.h"
#include "assert.h"
#include "Dentry.h"
#include "Inode.h"
#include "File.h"

Superblock::Superblock(size_t s_dev) :
    s_magic_(0), s_type_(0), s_dev_(s_dev), s_flags_(0), s_root_(0), s_mountpoint_(0)
{
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
  return s_mountpoint_;
}

void Superblock::setMountPoint(Dentry* mountpoint)
{
    s_mountpoint_ = mountpoint;
}

FileSystemType* Superblock::getFSType()
{
  return (FileSystemType*) s_type_;
}
