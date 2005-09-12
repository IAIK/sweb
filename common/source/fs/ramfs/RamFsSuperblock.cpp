// Projectname: SWEB
// Simple operating system for educational purposes
//
// Copyright (C) 2005  Chen Qiang
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "fs/ramfs/RamFsSuperblock.h"
#include "fs/ramfs/RamFsInode.h"
#include "console/kprintf.h"
#include "fs/ramfs/RamFsInode.h"
#include "assert.h"

#define ROOT_NAME "/"

//----------------------------------------------------------------------
RamFsSuperblock::RamFsSuperblock(Dentry* s_root) : Superblock(s_root)
{
  kprintfd("***** enters Constructor of the RamFsSuperblock\n");
  Dentry *root_dentry = 0;

  // create or find a root_dentry
  if (s_root)
  {
    Dentry* parent = s_root->getParent();
    root_dentry = new Dentry(parent);
      
    mounted_over_ = s_root;
    s_root_ = root_dentry;
  }
  else
  {
    kprintfd("init the ROOT_NAME\n");
    root_dentry = new Dentry(ROOT_NAME);
    mounted_over_ = 0;
  }
  s_root_ = root_dentry;

  // create the inode for the root_dentry
  Inode *root_inode = (Inode*)(new RamFsInode(this, I_DIR));
  int32 root_init = root_inode->mknod(root_dentry);
  assert(root_init == 0);

  // add the root_inode in the list
  all_inodes_.pushBack(root_inode);
  kprintfd("***** leaves Constructor of the RamFsSuperblock\n");
}

//----------------------------------------------------------------------
RamFsSuperblock::~RamFsSuperblock()
{
  kprintfd("***** start Destructor of RamFsSuperblock\n");
  assert(dirty_inodes_.empty() == true);

  uint32 num = all_inodes_.getLength();
      kprintfd("num = %d\n", num);
 
  for(uint32 counter = 0; counter < num; counter++)
  {
    kprintfd("SSSSSS\n");
    Inode* inode = all_inodes_.at(0);
    Dentry* dentry = inode->getDentry();
    all_inodes_.remove(inode);
    delete dentry;
    delete inode;
    kprintfd("loop\n");
  }
/*
  Dentry* current_dentry = s_root_;
  for(;;)
  {
    kprintfd("loop\n");
    if((current_dentry == s_root_) && (current_dentry->emptyChild() == true))
      break;
    
    uint32 num = current_dentry->getNumChild();
    kprintfd("dentry = %s, has child %d\n", current_dentry->getName(), num);
    kprintfd("current_dentry->emptyChild() = %d\n", current_dentry->emptyChild());
//    if(num > 0)
    if(current_dentry->emptyChild() == true)
    {
      kprintfd("ssssss\n");
      Inode* current_inode = current_dentry->getInode();
      kprintfd("delete dentry = %s\n", current_dentry->getName());
      current_dentry = current_dentry->getParent();
      kprintfd("hier\n");
      if(current_inode->rmdir() == INODE_DEAD)
        all_inodes_.remove(current_inode);
      else
        kprintfd("rmdir failed\n");
    }
    else
    {
      kprintfd("ssss\n");
      current_dentry = current_dentry->getChild(0);
      kprintfd("current_dentry = %s\n", current_dentry->getName());
    }
    kprintfd("SSSSSSSSS\n");
  }
  */
  
  kprintfd("***** all_inodes_.getLength = %d\n", all_inodes_.getLength());
  kprintfd("***** all_inodes_.empty = %d\n", all_inodes_.empty());
  assert(all_inodes_.empty() == true);
  kprintfd("***** end Destructor of RamFsSuperblock\n");
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

  all_inodes_.pushBack(inode);
}

//----------------------------------------------------------------------
void RamFsSuperblock::write_inode(Inode* inode)
{
  assert(inode);
  
  all_inodes_.pushBack(inode);
}

//----------------------------------------------------------------------
void RamFsSuperblock::delete_inode(Inode* inode)
{
  all_inodes_.remove(inode);
}

