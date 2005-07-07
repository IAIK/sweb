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
#include "List.h"
#include "Superblock.h"

#ifndef Dentry_h___
#define Dentry_h___

class Inode;
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
  Qstr()
  { 
    length_ = 0; 
    name_ = 0;
  }

  ~Qstr() { delete[] name_; }

  void set_name(char* name, uint32 length)
  {
  	length_ = length;
    name_ = new char(length);
    for(uint32 count = 0; count < length; count++)
      name_[count] = name[count];	
  }

  uint32 get_length() { return length_; }
  char* get_name() { return name_; }
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
  /// It increments if a inode linked with this dentry and derements if
  /// unlinked.
  int32 d_count_;

  /// The pointer to the inode related to this name. This field may be NULL,
  /// which indicates a negative entry, implying that the name is known not to
  /// exist.
  /// It can be a List of d_inode_ if it used to the link & unlink
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

  /// For a directory that has had a file-system mounted on it, this points to
  /// the root dentry of that file-system. For other dentries, this points back
  /// to the dentry itself.
  Dentry *d_mounts_; /* mount information */

  /// This is the inverse of d_mounts. For the root of a mounted file-system,
  /// this points to the dentry of the directory that it is mounted on. For
  /// other dentrys, this points to the dentry itself.
  Dentry *d_covers_;

  /// The d_name field contains the name of this entry, together with its hash
  /// value.
  Qstr *d_name_;
  
  /// This points to the super-block of the file-system on which the object
  /// refered to by the dentry resides. It is not clear why this is needed
  /// rather than using d_inode->i_sb.
  // Superblock * d_sb_; /* The root of the dentry tree */

public:

  void set_inode(Inode *inode) { d_inode_ = inode; }
  Inode* get_inode() { return d_inode_; }

  bool find_child(Dentry *entry) { return d_child_.is_included(entry); }
  bool empty_child() { return d_child_.is_empty(); }

  void increment_dcount() { d_count_++; }
  void decrement_dcount() { d_count_--; }

  void set_name(char* name, uint32 length) { d_name_->set_name(name, length); }
  char* get_name() { return d_name_->get_name(); }
  uint32 get_name_length() { return d_name_->get_length(); }

  /// This should compare the qstr with the all qstrs of the d_child_ list.
  /// It should return false if it exists the same qstr in the 
  /// list, return 0 if doesn't exist.
  virtual bool check_name(char *name, uint32 length);

  /// remove a child dentry from the d_child_ list.
  /// @child the child dentry of the curent dentry.
  virtual int32 d_child_remove(Dentry *child_dentry);
  
  /// insert a child dentry to the d_child_ list.
  /// @child the 
  virtual void d_child_insert(Dentry *child_dentry);

public:

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
  virtual void d_delete();
};


#endif // Dentry_h___
