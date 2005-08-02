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


#ifndef Inode_h___
#define Inode_h___

#include "types.h"
#include "fs/PointList.h"
#include "Dentry.h"
#include "File.h"

class Dentry;
class File;
class Superblock;

// three possible inode state bits:
#define I_UNUSED 0 // the unused inode state
#define I_DIRTY 1 // Dirty inodes are on the per-super-block s_dirty_ list, and
                  // will be written next time a sync is requested.

// three possible inode mode bits:
#define I_FILE 0
#define I_DIR  1
#define I_LNK  2

// The per-inode flags:
#define MS_NODEV 2 // If this inode is a device special file, it cannot be
                   // opend.

//-------------------------------------------------------------------------
/**
 * Inode
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
class Inode
{
 protected:
  /// The i_list_ linked list links inodes in various states. There is the
  /// inode_in_use list which lists unchanged inodes that are in active use.
  /// inode_unused which lists unused inodes, and the s_dirty_ of Superblock
  /// class store all the dirty inodes on the given file system.
  PointList<Inode> i_list_;

  /// The dentry of this inode. (dir)
  Dentry *i_dentry_;

  /// The dentry-PointList of this inode. (file)
  PointList<Dentry> i_dentry_link_;

  /// the reference count of the inode. if i_count_ is zero, it can be free
  /// the inode.
  uint32 i_count_;

  /// the number of the link of this inode.
  uint32 i_nlink_;

  /// reference of superblock
  Superblock *i_superblock_;

  /// current file size in bytes
  uint32 i_size_;

  /// The basic block size of inode.
  uint64 i_blksize_;

  /// The power of 2 that i_blksize_ is.
  uint32 i_blocks_;

  /// There are three possible inode mode bits: I_FILE, I_DIR, I_LNK
  uint32 i_mode_;

  /// There are three possible inode state bits: I_DIRTY, I_LOCK.
  uint32 i_state_;


 public:

  Inode(Superblock *super_block, uint32 inode_mode)
    { i_superblock_ = super_block, i_mode_ = inode_mode; }

  virtual ~Inode() {}

  //--------------------------------------------------------------------------
  // This methode are only meaningful on directory inodes.
  //--------------------------------------------------------------------------
  /// create is called when the VFS wants to create a file with the given name
  /// (in the dentry) in the given directory. The VFS will have already checked
  /// that the name doesn't exist, and the dentry passed will be a negative
  /// dentry meanging that the inode pointer will be NULL.If create successful,
  /// get a new empty inode from the cache with get_empty_inode, fill in the
  /// fields and insert it into the hash table with insert_inode_hash, mark it
  /// dirty, and instantiate it into the Dcache with d_instantiate.
  virtual int32 create(Dentry *) { return 0; }

  /// lookup should check if that name (given by the Dentry) exists in the
  /// directory (given by the inode) and should update the Dentry if it does.
  /// This involves finding and loading the inode. If the lookup failed to find
  /// anything, this is indicated by returning a negative value.
  virtual Dentry* lookup(Dentry *) {return 0;}

  /// The link method should make a hard link to by the dentry, which is in
  /// the directory refered to by the Inode.
  virtual int32 link(Dentry *) {return 0;}

  /// This should remove the name refered to by the Dentry from the directory
  /// referred to by the inode. It should d_delete the Dentry on success.
  virtual int32 unlink(Dentry *) {return 0;}

  /// This should create a symbolic link in the given directory with the given
  /// name having the given value. It should d_instantiate the new inode into
  /// the dentry on success.
  virtual int32 symlink(Inode */*inode*/, Dentry */*dentry*/, 
                        const char */*link_name*/) {return 0;}

  /// Create a directory with the given parent and name.
  virtual int32 mkdir(Dentry *) {return 0;}

  /// Remove the named directory (if empty) and d_delete the dentry.
  virtual int32 rmdir(Dentry *) {return 0;}

  /// Create a device special file with name and device number (the inode is
  /// the parent). Then d_instantiate the new inode into the dentry.
  virtual int32 mknod(Dentry *) {return 0;}

  /// The src_inode and src_entry refer to a directory and name that exist.
  /// rename should rename the object to have the parent and name given by the
  /// the prt_inode and dst_dentry. All generic checks, including that the new
  /// parent isn't a child of the old name, have already been done.
  virtual int32 rename(Inode */*src_inode*/, Dentry */*src_dentry*/,
                       Inode */*prt_inode*/, Dentry */*dst_dentry*/) {return 0;}
  //--------------------------------------------------------------------------

  /// The symbolic link referred to by the dentry is read and the value is
  /// copied into the user buffer (with copy_to_user) with a maximum length
  /// given by the intege.
  virtual int32 readlink(Dentry */*dentry*/, char*, int32 /*max_length*/) {return 0;}

  /// If the directory (parent dentry) have a directory and a name within that
  /// directory (child dentry) then the obvious result of following the name
  /// from the directory would arrive at the child dentry. 
  /// @param prt_dentry the parent dentry
  /// @param chd_dentry the child dentry
  virtual Dentry* follow_link(Dentry */*prt_dentry*/, Dentry */*chd_dentry*/) {return 0;}

  /// read the date of the inode
  virtual int32 readData(int32 /*offset*/, int32 /*size*/, int32 */*buffer*/) {return 0;}
};


#endif // Inode_h___


