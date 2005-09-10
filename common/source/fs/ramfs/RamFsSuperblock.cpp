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
  root_dentry->setInode(root_inode);

  // add the root_inode in the list
  all_inodes_.pushBack(root_inode);
  kprintfd("***** leaves Constructor of the RamFsSuperblock\n");
}
//----------------------------------------------------------------------
RamFsSuperblock::~RamFsSuperblock()
{
}

//----------------------------------------------------------------------
void RamFsSuperblock::read_inode(Inode* inode)
{
  assert(inode);
  assert(s_inode_used_.empty());

  all_inodes_.pushBack(inode);
}

//----------------------------------------------------------------------
void RamFsSuperblock::write_inode(Inode* /*inode*/)
{
}

//----------------------------------------------------------------------
void RamFsSuperblock::delete_inode(Inode* inode)
{
  all_inodes_.remove(inode);
}

