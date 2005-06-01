// Projectname: SWEB
// Simple operating system for educational purposes
//
// Copyright (C) 2004 Maria Mauerhofer
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

#ifndef Dentry_h___
#define Dentry_h___

#include "types.h"
#include "Inode.h"
#include "List.h"
#include "Superblock.h"

//-----------------------------------------------------------------------------
/**
 * Qstr - Quick string
 *
 * eases parameter passing, but more importantly saves "metadata" about the
 * string (i.e. length and the hash)
 */
class Qstr
{
protected:

  uint32 length_;
  char *name_;

public:

  Qstr(uint32 length, char *name)
  { length_ = length; name_ = name; }

  ~Qstr() {}
};
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
/**
 * Dentry
 *
 * The VFS layer does all management of path names of files, and converts them
 * into entries in the dentry before passing allowing the underlying
 * file-system to see them. The dentry object associates the component to its
 * corresponding inode.
 */

class Dentry
{
 protected:

  /// Dentry object usage count
  /// The count does NOT include the reference from the parent through the
  /// d_subdirs list, but does include the d_parent references from children.
  /// This implies that only leaf nodes in the cache may have a d_count of 0.
  /// These entries are linked together by the d_lru list as will be seen.
  int32 d_count_;

  /// There are currently two possible flags, both for use by specific
  /// file-system implementations. They are DCACHE_AUTOFS_PENDING and
  /// DCACHE_NFSFS_RENAMED.
  uint32 d_flags_;

  /// The pointer to the inode related to this name. This field may be NULL,
  /// which indicates a negative entry, implying that the name is known not to
  /// exist.
  Inode *d_inode_; /* Where the name belongs to - NULL is negative */

  /// This will point to the parent dentry. For the root of a file-system, or
  /// for an anonymous entry like that for a file, this points back to the
  /// containing dentry itself.
  Dentry *d_parent_; /* parent directory */

  /// This list_head is used to link together all the children of the d_parent_
  /// of this dentry.
  List<Dentry> d_child_; /* child of parent list */

  /// This is the head of the d_child list that links all the children of this
  /// dentry. Of course, elements may refer to file and not just
  /// sub-directories.
  List<Dentry> d_subdirs_; /* our children */

  /// As files (and some other file-system objects) may have multiple names in
  /// the file-system through multiple hard links, it is possible that multiple
  /// dentrys refer to the same inode. When this happens, the dentrys are
  /// linked on the d_alias field. The inode's i_dentry field is the head of
  /// this list.
  List<Dentry> d_alias_; /* inode alias list */

  /// For a directory that has had a file-system mounted on it, this points to
  /// the root dentry of that file-system. For other dentries, this points back
  /// to the dentry itself.
  Dentry * d_mounts_; /* mount information */

  /// This is the inverse of d_mounts. For the root of a mounted file-system,
  /// this points to the dentry of the directory that it is mounted on. For
  /// other dentrys, this points to the dentry itself.
  Dentry * d_covers_;

  /// This doubly linked list chains together the entries in one hash bucket.
  /// List d_hash_; /* lookup hash list */

  /// ???
  /// This provides a doubly linked list of unreferenced leaf nodes in the
  /// cache. The head of the list is the dentry_unused global variable. It is
  /// stored in Least Recently Used order. When other parts of the kernel need
  /// to reclaim memory or inodes, which may be locked up in unused entries in
  /// the dcache, they can call select_dcache which finds removable entries in
  /// the d_lru and prepares them to be removed by prune_dcache.
  /// List d_lru_; /* d_count = 0 LRU list */

  /// The d_name field contains the name of this entry, together with its hash
  /// value.
  Qstr *d_name_;

  /// This field is only used by underlying file-systems, which can presumably
  /// do whatever they want. The intention is to use it to record something
  /// about when this entry was last known to be valid to get some idea about
  /// when its validity might need to be checked again.
  // uint64 d_time; /* used by d_revalidate */

  /// This points to the super-block of the file-system on which the object
  /// refered to by the dentry resides. It is not clear why this is needed
  /// rather than using d_inode->i_sb.
  // Superblock * d_sb_; /* The root of the dentry tree */

  /// This is set to the current time in jiffies whenever the d_count_ reaches
  /// zero, but it is never used.
  // uint64 d_reftime_; /* last time referenced */

 public:

  Dentry() {}

  virtual ~Dentry();

  /// This method is called whenever a path lookup uses an entry in the dcache,
  /// in odrder to see if the entry is still valid. It is only needed if the
  /// file-system is likely to change without the VFS layer doing anything.
  /// @param dentry the input dentry.
  /// @param lookup_falgs gibes the flags relevant to this lookup.
  /// @return It returns 1 if the entry is can still be trusted, else 0. The
  /// default is to assume a return value of 1.
  // virtual int32 d_revalidate(Dentry *dentry, int lookup_flags) {return 0;}

  /// If the name is valid, a hash should be calculated (which should be the
  /// same for all equivalent names) and stored in the qstr argument.
  /// @param dentry is the dentry of the parent of the name
  /// @param name the name information
  /// @return If the name is not valid return an negative error code, if the
  /// name is validity return a canonical hash.
  virtual int32 d_hash (Dentry *dentry, Qstr *qstr) {return 0;}


  /// This should compare the two qstrs (again in the context of the dentry
  /// being their parent) to see if they are equivalent. It should return 0
  /// only if they are the same. Ordering is not important.
  virtual int32 d_compare(Dentry *dentry, Qstr *qstr1, Qstr *qstr2){return 0;}


  /// This is called when the reference count reaches zero, before the dentry
  /// is placed on the dentry_unused list.
  virtual void d_delete ();

  virtual void d_release (Dentry *dentry ) {}

// This is called just before a dentry is finally freed up. It can be
// used to release the d_fsdata if any.

  virtual void d_iput (Dentry *dentry , Inode *inode ) {}

// If defined, this is called instead of iput to release the inode when the
// dentry is being discarded. It should do the equivalent of iput plus
// anything else that it wants.

private:

};


#endif // Dentry_h___
