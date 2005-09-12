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

#include "types.h"
#include "Inode.h"
#include "fs/PointList.h"
#include "Superblock.h"

#ifndef Dentry_h___
#define Dentry_h___

class Inode;

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
  // int32 d_count_;

  /// The pointer to the inode related to this name.
  Inode *d_inode_;

  /// This will point to the parent dentry. For the root of a file-system, or
  /// for an anonymous entry like that for a file, this points back to the
  /// containing dentry itself.
  Dentry *d_parent_;

  /// This list_head is used to link together all the children of the dentry.
  PointList<Dentry> d_child_;

  /// This is the head of the d_child list that links all the children of this
  /// dentry. Of course, elements may refer to file and not just
  /// sub-directories.
  // PointList<Dentry> d_subdirs_;

  /// For a directory that has had a file-system mounted on it, this points to
  /// the root dentry of that file-system. For other dentries, this points back
  /// to the dentry itself.
  Dentry *d_mounts_;

  /// This is the inverse of d_mounts_. For the root of a mounted file-system,
  /// this points to the dentry of the directory that it is mounted on. For
  /// other dentrys, this points to the dentry itself.
  // Dentry *d_covers_;

  /// The d_name field contains the name of this entry, together with its hash
  /// value.
  char *d_name_;
  
public:

  void setInode(Inode *inode) { d_inode_ = inode; }
  void releaseInode() { d_inode_ = 0; }
  Inode* getInode() { return d_inode_; }
  
  /// return the parent of the dentry
  Dentry* getParent() { return d_parent_; }

  bool findChild(Dentry *dentry) { return d_child_.included(dentry); }
  int32 setChild(Dentry *dentry);
  bool emptyChild() { return d_child_.empty(); }
  uint32 getNumChild() { return d_child_.getLength(); }
  Dentry* getChild(uint32 index);

  void setName(char* name);
  char* getName();

  /// This should compare the name with the all names of the d_child_ list.
  /// It should return the Dentry if it exists the same name in the list, 
  /// return 0 if doesn't exist.
  virtual Dentry* checkName(const char* name);

  /// remove a chconst char* nameild dentry from the d_child_ list.
  /// @child the child dentry of the curent dentry.
  virtual int32 childRemove(Dentry *child_dentry);
  
  /// insert a child dentry to the d_child_ list.
  /// @child the 
  virtual void childInsert(Dentry *child_dentry);

public:

  /// Constructor of a new dentry.
  /// create a Dentry, the root-directory call this contructor.
  Dentry(char* name);

  /// Constructor of a new dentry.
  /// It muss to check the double name in the parent dentry before to call this
  /// contructor.
  Dentry(Dentry *parent);

  /// It muss to delete the pointer in all parent-dentry before to call this
  /// destructor
  virtual ~Dentry();

  /// This method is called whenever a path lookup uses an entry in the dcache,
  /// in odrder to see if the entry is still valid. It is only needed if the
  /// file-system is likely to change without the VFS layer doing anything.
  /// @param dentry the input dentry.
  /// @param lookup_falgs gibes the flags relevant to this lookup.
  /// @return It returns 1 if the entry is can still be trusted, else 0. The
  /// default is to assume a return value of 1.
  // virtual int32 d_revalidate(Dentry *dentry, int lookup_flags) {return 0;}

  /// This is called when the reference count reaches zero, before the dentry
  /// is placed on the dentry_unused list.
  // virtual void d_delete();
};


#endif // Dentry_h___
