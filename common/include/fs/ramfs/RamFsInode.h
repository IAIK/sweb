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

class BufferHead;
// class File;
// class Page;

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

  //--------------------------------------------------------------------------
  // This methode are only meaningful on directory inodes.
  //--------------------------------------------------------------------------
  /// create is called when the VFS wants to create a file with the given name
  /// (in the dentry) in the given directory. The VFS will have already checked
  /// that the name doesn't exist, and the dentry passed will be a negative
  /// dentry meanging that the inode pointer will be NULL.
  virtual int32 create(Dentry *dentry);

    /// lookup should check if that name (given by the Dentry) exists in the
    /// directory (given by the inode) and should update the Dentry using d_add
    /// if it does. This involves finding and loading the inode. If the lookup
    /// failed to find anything, this is indicated by returning a negative
    /// Dentry, with an inode pointer of NULL.
    virtual Dentry* lookup(Inode *inode, Dentry *dentry) {return((Dentry*)0);}

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
    virtual int32 symlink(Inode *inode, Dentry *dentry, const char *link_name)
    {return 0;}

  /// Create a directory with the given parent and name.
  virtual int32 mkdir(Dentry *dentry);

  /// Remove the named directory (if empty) and d_delete the dentry.
  virtual int32 rmdir(Dentry *dentry);

  /// Create a device special file with name and device number (the inode is
  /// the parent).
  virtual int32 mknod(Dentry *dentry);

    /// The src_inode and src_entry refer to a directory and name that exist.
    /// rename should rename the object to have the parent and name given by the
    /// the prt_inode and dst_dentry. All generic checks, including that the new
    /// parent isn't a child of the old name, have already been done.
    virtual int32 rename(Inode *src_inode, Dentry *src_dentry,
        Inode *prt_inode, Dentry *dst_dentry) {return 0;}
    //--------------------------------------------------------------------------

    /// The symbolic link referred to by the dentry is read and the value is
    /// copied into the user buffer (with copy_to_user) with a maximum length
    /// given by the intege.
    virtual int32 readlink(Dentry *dentry, char*, int32 max_length) {return 0;}

    /// If the directory (parent dentry) have a directory and a name within that
    /// directory (child dentry) then the obvious result of following the name
    /// from the directory would arrive at the child dentry. If an inode requires
    /// some other, non-obvious, result - s do symbolic links - the inode should
    /// provide a follow_link method to return the appropriate new dentry.
    /// @prt_dentry the parent dentry
    /// @chd_dentry the child dentry
    /// @lookup_flags a number of LOOKUP flags
    virtual Dentry* follow_link(Dentry *prt_dentry, Dentry *chd_dentry,
        uint32 lookup_flags = 0) {return((Dentry*)0);}

    /// This method is used to find the device block that holds a given block of
    /// a file. get_block should initialise the b_dev and b_blocknr field of the
    /// buffer_head, and should possibly modify the b_state flags.
    /// @inode the file that the block hold
    /// @block_number the file offset divided by file-system block size
    /// @b_state the buffer state flags.
    virtual int32 get_block(Inode *inode, int64 block_number,
        BufferHead *buffer_head, int32 b_state) {return 0;}

    /// It is needed for memory mapping of files, for using the send file system
    /// call.
    // virtual int32 read_page(File *, Page *) { return 0; }
    // virtual int32 wirte_page(File *, Page *) { return 0; }
    // virtual int32 flush_page(Inode *, Page *, uint64) { return 0; }

  int32 readData(int32 offset, int32 size, int32 *buffer);
};


#endif // Inode_h___


