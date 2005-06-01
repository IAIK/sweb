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

#ifndef Vfsmount_h___
#define Vfsmount_h___

#include "Superblock.h"
#include "List.h"
#include "types.h"
#include "Dentry.h"

//---------------------------------------------------------------------------
/**
 * Vfsmount
 *
 * Definitions for mount interface. This describes the in the kernel
 * build linkedlist with mounted filesystems.
 */
class Vfsmount
{
 protected:

  /// Points to the parent filesystem on which this filesystem is mounted on.
  Vfsmount *mnt_parent_;

  /// Points to the Dentry of the mount directory of this filesystem.
  Dentry *mnt_mountpoint_;

  /// Points to the Dentry of the root directory of this filesystem.
  Dentry *mnt_root_;

  /// Points to the superblock object of this filesystem.
  Superblock *mnt_sb_;

  /// The mnt_flags_ field of the descriptor stores the value of several flags
  /// that specify how some kinds of files in the mounted filesystem are
  /// handled.
  int32 mnt_flags_;

  /// Head of the parent list of descriptors (relative to this filesystem).
  List *mnt_mounts_;

  /// Pointers for the parent list of descriptors (relative to the parent
  /// filesystem).
  List *mnt_child_;

 public:

  Vfsmount();

  virtual ~Vfsmount();

  void put_mnt(Vfsmount *mnt);

  void remove_mnt(Vfsmount *mnt);

  Vfsmount* get_mnt();
};


#endif // Vfsmount_h___


