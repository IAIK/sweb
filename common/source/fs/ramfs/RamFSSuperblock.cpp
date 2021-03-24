#include "fs/FileDescriptor.h"
#include "fs/ramfs/RamFSSuperblock.h"
#include "fs/ramfs/RamFSInode.h"
#include "fs/ramfs/RamFSFile.h"
#include "fs/Dentry.h"
#include "assert.h"

#include "console/kprintf.h"
#include "console/debug.h"
#define ROOT_NAME "/"

RamFSSuperblock::RamFSSuperblock(uint32 s_dev) :
    Superblock(s_dev)
{
  Inode *root_inode = createInode(I_DIR);
  s_root_ = new Dentry(root_inode);
  assert(root_inode->mknod(s_root_) == 0);
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

Inode* RamFSSuperblock::createInode(uint32 type)
{
    debug(RAMFS, "createInode, type: %x\n", type);
    auto inode = new RamFSInode(this, type);

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

  File* file = inode->open(flag);
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
  int32 tmp = inode->release(file);

  debug(RAMFS, "remove the fd num: %d\n", fd->getFd());
  if (inode->getNumOpenedFile() == 0)
  {
    used_inodes_.remove(inode);
  }
  delete fd;

  return tmp;
}
