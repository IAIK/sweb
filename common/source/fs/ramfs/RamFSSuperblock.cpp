/**
 * @file RamFSSuperblock.cpp
 */

#include "fs/FileDescriptor.h"
#include "fs/ramfs/RamFSSuperblock.h"
#include "fs/ramfs/RamFSInode.h"
#include "fs/ramfs/RamFSFile.h"
#include "fs/Dentry.h"
#include "assert.h"

#include "console/kprintf.h"
#include "console/debug.h"
#define ROOT_NAME "/"

RamFSSuperblock::RamFSSuperblock(Dentry* s_root, uint32 s_dev) :
    Superblock(s_root, s_dev)
{
  Dentry *root_dentry = new Dentry(ROOT_NAME);

  if (s_root)
  {
    // MOUNT
    mounted_over_ = s_root;
  }
  else
  {
    // ROOT
    mounted_over_ = root_dentry;
  }
  s_root_ = root_dentry;

  // create the inode for the root_dentry
  Inode *root_inode = (Inode*) (new RamFSInode(this, I_DIR));
  int32 root_init = root_inode->mknod(root_dentry);
  assert(root_init == 0);

  // add the root_inode in the list
  all_inodes_.push_back(root_inode);
}

RamFSSuperblock::~RamFSSuperblock()
{
  assert(dirty_inodes_.empty() == true);

  for (FileDescriptor* fd : s_files_)
  {
    delete fd->getFile();
    delete fd;
  }
  s_files_.clear();

  for (Inode* inode : all_inodes_)
  {
    delete inode->getDentry();
    delete inode;
  }
  all_inodes_.clear();
}

Inode* RamFSSuperblock::createInode(Dentry* dentry, uint32 type)
{
  Inode *inode = (Inode*) (new RamFSInode(this, type));
  assert(inode);
  if (type == I_DIR)
  {
    debug(RAMFS, "createInode: I_DIR\n");
    int32 inode_init = inode->mknod(dentry);
    assert(inode_init == 0);
  }
  else if (type == I_FILE)
  {
    debug(RAMFS, "createInode: I_FILE\n");
    int32 inode_init = inode->mkfile(dentry);
    assert(inode_init == 0);
  }

  all_inodes_.push_back(inode);
  return inode;
}

int32 RamFSSuperblock::readInode(Inode* inode)
{
  assert(inode);

  if (ustl::find(all_inodes_, inode) == all_inodes_.end())
  {
    all_inodes_.push_back(inode);
  }
  return 0;
}

void RamFSSuperblock::writeInode(Inode* inode)
{
  assert(inode);

  if (ustl::find(all_inodes_, inode) == all_inodes_.end())
  {
    all_inodes_.push_back(inode);
  }
}

void RamFSSuperblock::delete_inode(Inode* inode)
{
  all_inodes_.remove(inode);
}

int32 RamFSSuperblock::createFd(Inode* inode, uint32 flag)
{
  assert(inode);

  File* file = inode->link(flag);
  FileDescriptor* fd = new FileDescriptor(file);
  s_files_.push_back(fd);
  FileDescriptor::add(fd);

  if (ustl::find(used_inodes_, inode) == used_inodes_.end())
  {
    used_inodes_.push_back(inode);
  }

  return (fd->getFd());
}

int32 RamFSSuperblock::removeFd(Inode* inode, FileDescriptor* fd)
{
  assert(inode);
  assert(fd);

  s_files_.remove(fd);
  FileDescriptor::remove(fd);

  File* file = fd->getFile();
  int32 tmp = inode->unlink(file);

  debug(RAMFS, "remove the fd num: %d\n", fd->getFd());
  if (inode->getNumOpenedFile() == 0)
  {
    used_inodes_.remove(inode);
  }
  delete fd;

  return tmp;
}
