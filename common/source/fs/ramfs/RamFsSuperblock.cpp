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

//----------------------------------------------------------------------
RamFsSuperblock::~RamFsSuperblock()
{
}

//----------------------------------------------------------------------
void RamFsSuperblock::read_inode(Inode* inode)
{
  assert(inode);

  all_inodes_.push_end(inode);
  s_inode_used_.push_end(inode);
}

//----------------------------------------------------------------------
void RamFsSuperblock::write_inode(Inode* inode)
{
}

//----------------------------------------------------------------------
void RamFsSuperblock::delete_inode(Inode* inode)
{
  all_inodes_.remove(inode);
}

