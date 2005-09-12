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

#ifndef RamFsInode_h___
#define RamFsInode_h___

#include "types.h"
#include "fs/Superblock.h"
#include "fs/PointList.h"
#include "fs/Inode.h"

//-------------------------------------------------------------------------
/**
 * RamFsInode
 *
 * All information needed by the filesystem to handle a file is included in a
 * data class called an inode. A filename is a casually assigned label that
 * can be changed, but the inode is unique to the file and remains the same
 * as long as the file exists.
 *
 * The path is through the inode hash table. Each inode is hashed (to an 8 bit
 * number) based on the address of the file-system's superblock and the inode
 * number. Inodes with the same hash value are then chained together in a
 * doubly linked list.
 */
class RamFsInode : public Inode
{
protected:

  /// the data of the inode
  int32* data_;
  
public:

  RamFsInode(Superblock *super_block, uint32 inode_mode);

  virtual ~RamFsInode();

  /// Create a directory with the given dentry.
  virtual int32 create(Dentry *dentry);

  /// lookup should check if that name (given by the Dentry) exists in the
  /// directory (given by the inode) and should update the Dentry if it does.
  /// This involves finding and loading the inode. If the lookup failed to find
  /// anything, this is indicated by returning a negative value.
  virtual Dentry* lookup(const char *name);

  /// The link method should make a hard link to the name referred to by the
  /// denty, which is in the directory refered to by the Inode. 
  /// @param dentry before link the dentry, the dentry muss be created.
  virtual int32 link(Dentry *dentry);

  /// This should remove the name refered to by the Dentry from the directory
  /// referred to by the inode.
  /// @param dentry after unlinked the dentry, the dentry muss be destructed.
  virtual int32 unlink(Dentry *dentry);

  /// This should create a symbolic link in the given directory with the given
  /// name having the given value. It should d_instantiate the new inode into
  /// the dentry on success.
  virtual int32 symlink(Inode */*inode*/, Dentry */*dentry*/, 
                        const char */*link_name*/) {return 0;}

  /// Create a directory with the given dentry. It is only used to with directory.
  virtual int32 mkdir(Dentry *dentry);

  /// Remove the directory (if the sub_dentry is empty).
  virtual int32 rmdir();

  /// Create a directory with the given dentry.
  virtual int32 mknod(Dentry *dentry);

  /// The src_inode and src_entry refer to a directory and name that exist.
  /// rename should rename the object to have the parent and name given by the
  /// the prt_inode and dst_dentry. All generic checks, including that the new
  /// parent isn't a child of the old name, have already been done.
  virtual int32 rename(Inode */*src_inode*/, Dentry */*src_dentry*/,
                       Inode */*prt_inode*/, Dentry */*dst_dentry*/) {return 0;}

  /// The symbolic link referred to by the dentry is read and the value is
  /// copied into the user buffer (with copy_to_user) with a maximum length
  /// given by the intege.
  virtual int32 readlink(Dentry */*dentry*/, char*, int32 /*max_length*/) {return 0;}

  /// If the directory (parent dentry) have a directory and a name within that
  /// directory (child dentry) then the obvious result of following the name
  /// from the directory would arrive at the child dentry. If an inode requires
  /// some other, non-obvious, result - s do symbolic links - the inode should
  /// provide a follow_link method to return the appropriate new dentry.
  /// @prt_dentry the parent dentry
  /// @chd_dentry the child dentry
  /// @lookup_flags a number of LOOKUP flags
  virtual Dentry* followLink(Dentry */*prt_dentry*/, Dentry */*chd_dentry*/) {return 0;}

  virtual int32 readData(int32 offset, int32 size, int32 *buffer);
  virtual int32 writeData(int32 offset, int32 size, int32 *buffer);
};


#endif // Inode_h___


