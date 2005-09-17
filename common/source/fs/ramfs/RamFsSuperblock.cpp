// Projectname: SWEB
// Simple operating system for educational purposes

#include "fs/ramfs/RamFsSuperblock.h"
#include "fs/ramfs/RamFsInode.h"
#include "fs/ramfs/RamFsFile.h"
#include "fs/Dentry.h" 
#include "assert.h"

#define ROOT_NAME "/"

//----------------------------------------------------------------------
RamFsSuperblock::RamFsSuperblock(Dentry* s_root) : Superblock(s_root)
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
  Inode *root_inode = (Inode*)(new RamFsInode(this, I_DIR));
  int32 root_init = root_inode->mknod(root_dentry);
  assert(root_init == 0);

  // add the root_inode in the list
  all_inodes_.pushBack(root_inode);
}

//----------------------------------------------------------------------
RamFsSuperblock::~RamFsSuperblock()
{
  assert(dirty_inodes_.empty() == true);

  uint32 num = all_inodes_.getLength();
  for(uint32 counter = 0; counter < num; counter++)
  {
    Inode* inode = all_inodes_.at(0);
    Dentry* dentry = inode->getDentry();
    all_inodes_.remove(inode);

    if (dentry)
    {
      delete dentry;
    }
    delete inode;
  }

  assert(all_inodes_.empty() == true);
}

//----------------------------------------------------------------------
Inode* RamFsSuperblock::createInode(Dentry* dentry, uint32 mode)
{
  Inode *inode = (Inode*)(new RamFsInode(this, mode));
  int32 inode_init = inode->mknod(dentry);
  assert(inode_init == 0);

  all_inodes_.pushBack(inode);
  return inode;
}

//----------------------------------------------------------------------
void RamFsSuperblock::read_inode(Inode* inode)
{
  assert(inode);

  if (!all_inodes_.included(inode))
  {
    all_inodes_.pushBack(inode);
  }
}

//----------------------------------------------------------------------
void RamFsSuperblock::write_inode(Inode* inode)
{
  assert(inode);

  if (!all_inodes_.included(inode))
  {
    all_inodes_.pushBack(inode);
  }
}

//----------------------------------------------------------------------
void RamFsSuperblock::delete_inode(Inode* inode)
{
  all_inodes_.remove(inode);
}

