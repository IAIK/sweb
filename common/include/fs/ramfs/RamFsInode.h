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
#include "../Superblock.h"
#include "fs/PointList.h"

class FileLock;
class VMArea;
class WaitQueue;
class Swmaphore;
class Dentry;
class BufferHead;
class Semaphore;
class File;
class Page;

// three possible inode state bits:
#define I_DIRTY 1 // Dirty inodes are on the per-super-block s_dirty_ list, and
// will be written next time a sync is requested.
#define I_LOCK 2 // Inodes are locked while they are being created, read oder
// written.
#define I_FREEING 4 // An inode is has this state when the reference count and
// link count have both reached zero. This seems to be only
// used by fat file-system.

// The per-inode flags:
#define MS_NODEV 2 // If this inode is a device special file, it cannot be
// opend.

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
class RamFsInode
{
  protected:
    //--------------------------------------------------------------------------
    // list of the inode
    //--------------------------------------------------------------------------
    /// The i_hash_ linked list links together all inodes which hash to the same
    /// hash bucket. Hash values are based on the address of the superblock
    /// class, and the inode number of the inode.
    /// PointList i_hash_;

    /// The i_list_ linked list links inodes in various states. There is the
    /// inode_in_use list which lists unchanged inodes that are in active use.
    /// inode_unused which lists unused inodes, and the s_dirty_ of Superblock
    /// class store all the dirty inodes on the given file system.
    PointList<Inode> *i_list_;

    /// The i_dentry list is a list of all class Dentrys that refer to this
    /// inode. They are linked together with the d_alias_ field of the Dentry.
    PointList<Inode> *i_dentry_;
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    // Elementar
    //--------------------------------------------------------------------------
    /// the reference count of the inode. if i_count_ is zero, it can be free 
    /// the inode.
    uint64 i_count_;

    Superblock *i_superblock_;

    /// the data size of a inode.
    // off_t i_size_;

    /// The are three passible inode state bits: I_DIRTY, I_LOCK, I_FREEING.
    uint32 i_state_;

    /// This points to the list of class FileLock that impose locks in this
    /// inode.
    FileLock *i_flock_;
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    // memory variable
    //--------------------------------------------------------------------------
    /// All of the WMArea class that describe mapping of an inode are linked
    /// together with the vm_next_share and vm_pprev_share pointers. This i_mmap_
    /// pointer points into that list.
    VMArea *i_mmap_;

    /// If this is positive, it counts the number of clients (files or memory
    /// maps) which have write access. If negative, then the absolute value of
    /// this number counts the number of VM_DENYWRITE mappings that are current.
    /// Otherwise it is 0, and onbody is trying to write or trying to stop others
    /// from writing.
    int32 i_write_count_;
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    // inode synchronization
    //--------------------------------------------------------------------------
    /// This is a queue of processes that are waiting for i_sem_ swmaphore on the
    /// inode.
    WaitQueue *i_wait_;

    /// This swmaphore guards changes to the inode. Any code that wants to make
    /// non-atomic access to the inode (i.e. two related accesses with the
    /// possibility of sleeping inbetween) must first claim this semaphore. This
    /// includes such things as allocating and deallocating blocks and searching
    /// through directories.
    Semaphore *i_sem_;
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    // Quota management
    //--------------------------------------------------------------------------
    /// to limit the number of inode
    // Dquot *i_dquot_[MAXQUOTAS];
    //--------------------------------------------------------------------------

    int32* data_;

  public:

    RamFsInode();

    virtual ~RamFsInode();

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
    virtual int32 create(Inode *inode, Dentry *dentry) { return 0; }

    /// lookup should check if that name (given by the Dentry) exists in the
    /// directory (given by the inode) and should update the Dentry using d_add
    /// if it does. This involves finding and loading the inode. If the lookup
    /// failed to find anything, this is indicated by returning a negative
    /// Dentry, with an inode pointer of NULL.
    virtual Dentry* lookup(Inode *inode, Dentry *dentry) {return((Dentry*)0);}

    /// The link method should make a hard link from the name refered to by the
    /// src_dentry to the name referred to by the dst_denty, which is in the
    /// directory refered to by the Inode.
    virtual int32 link(Dentry *src_dentry, Inode *inode, Dentry *dst_dentry)
    {return 0;}

    /// This should remove the name refered to by the Dentry from the directory
    /// referred to by the inode. It should d_delete the Dentry on success.
    virtual int32 unlink(Inode *inode, Dentry *dentry) {return 0;}

    /// This should create a symbolic link in the given directory with the given
    /// name having the given value. It should d_instantiate the new inode into
    /// the dentry on success.
    virtual int32 symlink(Inode *inode, Dentry *dentry, const char *link_name)
    {return 0;}

    /// Create a directory with the given parent and name.
    virtual int32 mkdir(Inode *inode, Dentry *dentry) {return 0;}

    /// Remove the named directory (if empty) and d_delete the dentry.
    virtual int32 rmdir(Inode *inode, Dentry *dentry) {return 0;}

    /// Create a device special file with the given parent, name and device
    /// number. Then d_instantiate the new inode into the dentry.
    virtual int32 mknod(Inode *inode, Dentry *dentry, int32 dev_num) {return 0;}

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
    virtual int32 read_page(File *, Page *) { return 0; }
    virtual int32 wirte_page(File *, Page *) { return 0; }
    virtual int32 flush_page(Inode *, Page *, uint64) { return 0; }

    int32 readData(int32 offset, int32 size, int32 *buffer);
};


#endif // Inode_h___


