
//
// CVS Log Info for $RCSfile: Superblock.h,v $
//
// $Id: Superblock.h,v 1.6 2005/07/21 18:07:03 davrieb Exp $
// $Log: Superblock.h,v $
// Revision 1.5  2005/07/16 13:36:29  davrieb
// rename file.h and file.cpp to File.h and File.cpp
//
// Revision 1.4  2005/07/16 13:22:00  davrieb
// rrename List in fs to PointList to avoid name clashes
//
// Revision 1.3  2005/07/07 12:31:19  davrieb
// add ramfs and all changes it caused
//
// Revision 1.2  2005/06/01 09:20:36  davrieb
// add all changes to fs
//
// Revision 1.1  2005/05/10 16:42:32  davrieb
// add first attempt to write a virtual file system
//
//

#ifndef Superblock_h___
#define Superblock_h___

#include "types.h"
#include "Dentry.h"
#include "fs/PointList.h"
#include "Inode.h"
#include "File.h"

class Iattr;
class Statfs;
class WaitQueue;
class FileSystemType;

//-----------------------------------------------------------------------------
/**
 * Superblock
 *
 * The first block of the virtual-file-system. It contains for instance the
 * configuration of the file system. Sotre information concerning a mounted
 * filesystem. For disk-based filesystems, this object usually corresponds
 * to a filesystem control block stored on disk.
 */
class Superblock
{
protected:

  /// The file system type.
  FileSystemType *s_type_;

  /// The device that this file-system is mounted on.
  // uint32 s_dev_;

  /// This is a list of flags which are logically with the flags in each
  /// inode to detemine certain behaviours. There is one flag which applies
  /// only to the whole file-system.
  /// exp: MS_RDONLY
  /// A file-system with the flag set has been mounted read-only. No writing
  /// be permitted, and no indirect modification, such as mount time in the
  /// super-block or access times on files, will be made.
  uint64 s_flags_;

  /// This is a class Dentry which refers to the root of the file-system. It is
  /// normally created by loading the root iode from the file-system, and
  /// passing it to d_alloc_root. This dentry will get spliced into the Dcache
  /// by the mount command.
  Dentry *s_root_;

  /// The old Dentry of the mount point of a mounted file system
  Dentry *mounted_over_;

  /// A list of dirty inodes.
  PointList<Inode> s_inode_dirty_;

  /// A list of used inodes.
  PointList<Inode> s_inode_used_;

  /// This is a list of files (linked on f_list) of open files on this
  /// file-system. It is used, for example, to check if there are any files
  /// open for write before remounting the file-system as read-only.
  PointList<File> s_files_;

  //--------------------------------------------------------------------------
  // SYNCHRONIZATION
  //--------------------------------------------------------------------------
  /// This indicates whether the super-block is currently locked. It is
  /// managed by lock_super and unlock_super.
  uint8 s_lock_;

  /// This is a queue of processes that are waiting for the s_lock_ lock on
  /// the super-block.
  WaitQueue *s_wait_;
  //--------------------------------------------------------------------------
public:
  void insertInodeUsed(Inode *inode) { s_inode_used_.push_end(inode); }

public:

  Superblock(Dentry* s_root) :
    mounted_over_(0)
    { s_root_ = s_root; }

  virtual ~Superblock();

  /// This method is called to read a specific inode from a mounted
  /// file-system. It is only called from get_new_inode.
  virtual void read_inode(Inode* inode) {}

  /// This method is called to write a specific inode to a mounted file-system,
  /// and gets called on inodes which have been marked dirty with
  /// mark_inode_dirty.
  virtual void write_inode(Inode* inode) {}

  /// This method is called whenever the reference count on an inode is
  /// decreased put_inode called before the i_count field is decreased, so if
  /// put_inode wants to check if this is the last reference, it should check
  /// if i_count is 1 or not. This method used it to do some special handling
  /// when the last reference to the inode is release. i.e. when i_count is 1
  /// and is about to be come zero.
  virtual void put_inode(Inode* inode) {}

  /// This method is called whenever the reference count on an inode reaches 0,
  /// and it is found that the link count (i_nlink= is also zero. It si
  /// presumed that the file-system will deal with this situation be
  /// invalidating the inode in the file-system and freeing up any resourses
  /// used.
  virtual void delete_inode(Inode* inode);

  /// This is called when inode attributed are changed, the argument class
  /// Iattr* pointing to the new set of attributes. If the file-system does
  /// not define this method (i.e. it is NULL) then VFS uses the routine
  /// (inode_change_ok) which implements POSIX standard attributes
  /// verification. Then VFS marks the inode as dirty. If the file-system
  /// implements its own notify_change then it should call mark_inode_dirty
  /// (Inode).
  virtual int32 notify_change(Dentry* dentry, Iattr* iattr) { return 0; }

  /// This method is called with super-block lock held. A typical
  /// implementation would free file-system-private resources specific for
  /// this mount instance, such as inode bitmaps, block bitmaps, a buffer
  /// header containing super-block and decrement mount hold count if the
  /// file-system is implemented as a dynamically loadable module.
  /// Implementation: destructure of the Superblock.
  virtual void put_super() {}

  /// This method called when VFS decides that the super-block needs to be
  /// written to disk. It check the SuperBlock->s_dirty_, if it is true write
  /// the super block return to the Disc.
  virtual void write_super() {}

  /// This method is needed to implement statfs(2) system call
  virtual int32 statfs(Superblock*, Statfs*, int32) { return 0; }

  /// This method called when file-system is being remounted, i.e. if the
  /// MS_REMOUNT flag is specified with the mount system call. This can be used
  /// to change various mount options without unmounting the file-system. A
  /// common usage is to change a readonly file-system into a writable.
  virtual int32 remount_fs(Superblock*, int32*, char*) { return 0; }

  /// Optional method, call when VFS clears the inode. This is needed (at
  /// least) by file-system which attaches kmalloced data to the inode
  /// sturcture, as particularly might be the case for file-systems using the
  /// generic_ip field in class Inode.
  virtual void clear_inode(Inode* inode) {}

  /// This method is called early in the unmounting process if the MNT_FORCE
  /// flag was given to umount. The intentions is that it should cause any
  /// incomplete transaction on the file-system to fail quickly rather than
  /// block waiting on some external event such as a remote server responding.
  virtual void umount_begin(Superblock* super_block) {}
};
//-----------------------------------------------------------------------------

#endif // Superblock_h___

