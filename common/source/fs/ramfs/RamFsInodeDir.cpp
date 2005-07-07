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

#include "ramfs/RamFsInodeDir.h"
#include "Inode.h"
#include "Dentry.h"
#include "Superblock.h"
#include "assert.h"

//---------------------------------------------------------------------------
RamFsInodeDir::RamFsInodeDir(Superblock *super_block) : Inode(super_block, I_DIR)
{
  i_size_ = 0;
  i_state_ = 0;
  super_block->insertInodeUsed(this);
}
//---------------------------------------------------------------------------
RamFsInodeDir::~RamFsInodeDir()
{
  //TODO: update in the super_block used list / or before
}

//---------------------------------------------------------------------------
void RamFsInodeDir::unlink(Dentry *dentry)
{
/*
  Inode *inode = dentry->get_inode();
  if(inode)
    dentry->d_delete();
*/
}
//---------------------------------------------------------------------------
int32 RamFsInodeDir::mknod(Dentry *dentry)
{
  i_dentry_ = dentry;
  dentry->set_inode(this);
  dentry->increment_dcount();
  return 0;
}
//---------------------------------------------------------------------------
int32 RamFsInodeDir::mkdir(Dentry *dentry)
{
  return(mknod(dentry));
}
//---------------------------------------------------------------------------
int32 RamFsInodeDir::create(Dentry *dentry)
{
  return(mknod(dentry));
}

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
int32 RamFsInodeDir::rmdir(Dentry *child_dentry)
{
  if(i_dentry_->find_child(child_dentry))
  {
    // has the child_dentry got child dentry? 
    if(child_dentry->empty_child() == true)
    //  unlink(dentry);
    {
      Inode *inode = child_dentry->get_inode();
      if(inode)
        child_dentry->d_delete();
    }
  }
  else
    return -1;
  return 0;

}
