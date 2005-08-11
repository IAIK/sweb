// Projectname: SWEB
// Simple operating system for educational purposes
//
// Copyright (C) 2005  David Riebenbauer
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

//
// CVS Log Info for $RCSfile: VfsMount.cpp,v $
//
// $Id: VfsMount.cpp,v 1.1 2005/08/11 16:46:57 davrieb Exp $
// $Log$
//

#include "fs/VfsMount.h"


//----------------------------------------------------------------------
VfsMount::VfsMount() :
  mnt_parent_(0),
  mnt_mountpoint_(0),
  mnt_root_(0),
  mnt_sb_(0),
  mnt_flags_(0)
{
}

//----------------------------------------------------------------------
VfsMount::VfsMount(VfsMount* parent, Dentry * mountpoint, Dentry* root,
    Superblock* superblock, int32 flags) :
  mnt_parent_(parent),
  mnt_mountpoint_(mountpoint),
  mnt_root_(root),
  mnt_sb_(superblock),
  mnt_flags_(flags)
{
}

//----------------------------------------------------------------------
VfsMount::~VfsMount()
{
}

//----------------------------------------------------------------------
VfsMount const *VfsMount::getParent() const
{
  return mnt_parent_;
}

//----------------------------------------------------------------------
Dentry const *VfsMount::getMountpoint() const
{
  return mnt_mountpoint_;
}

//----------------------------------------------------------------------
Dentry const *VfsMount::getRoot() const
{
  return mnt_root_;
}

//----------------------------------------------------------------------
Superblock const *VfsMount::getSuperblock() const
{
  return mnt_sb_;
}

//----------------------------------------------------------------------
int32 VfsMount::getFlags() const
{
  return mnt_flags_;
}

