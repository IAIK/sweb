#include "fs/ramfs/RamFSSuperblock.h"

#include "fs/Dentry.h"
#include "fs/FileDescriptor.h"
#include "fs/ramfs/RamFSFile.h"
#include "fs/ramfs/RamFSInode.h"
#include "fs/ramfs/RamFSType.h"

#include "assert.h"
#include "debug.h"

#define ROOT_NAME "/"

RamFSSuperblock::RamFSSuperblock(RamFSType* fs_type, uint32 s_dev) :
    Superblock(fs_type, s_dev)
{
  Inode *root_inode = createInode(I_DIR);
  s_root_ = new Dentry(root_inode);
  assert(root_inode->mkdir(s_root_) == 0);
}

RamFSSuperblock::~RamFSSuperblock()
{
  assert(dirty_inodes_.empty() == true);

  releaseAllOpenFiles();

  deleteAllInodes();
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

  if (eastl::find(all_inodes_.begin(), all_inodes_.end(), inode) == all_inodes_.end())
  {
    all_inodes_.push_back(inode);
  }
  return 0;
}

void RamFSSuperblock::writeInode(Inode* inode)
{
  assert(inode);

  if (eastl::find(all_inodes_.begin(), all_inodes_.end(), inode) == all_inodes_.end())
  {
    all_inodes_.push_back(inode);
  }
}

void RamFSSuperblock::deleteInode(Inode* inode)
{
  all_inodes_.remove(inode);
  delete inode;
}
