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

#ifndef FileSystemType_h___
#define FileSystemType_h___

#include "Superblock.h"

/// File system flag indicating if the system in question requires an device.
#define FS_REQUIRES_DEV   0x0001 // located on a physical disk device
#define FS_NOMOUNT        0x0010 // Filesystem has no mount point

/// The maximal number of file system types.
#define MAX_FILE_SYSTEM_TYPES 16

/**
 * FileSystemType
 *
 */
class FileSystemType
{
public:

  typedef Superblock *(*ReadSuper) (struct Superblock *, void *, int32);

protected:

  const char* fs_name_;

  int32 fs_flags_;

//  Superblock *(*readsuper) (struct Superblock *, void *, int32);
  ReadSuper read_super_;

 public:

  FileSystemType();

  FileSystemType(const char* fs_name);

  ~FileSystemType();

  const char* getFSName() const;

  void setFSName(const char* fs_name);

  int32 getFSFlags() const;

  void setFSFlags(int32 fs_flags);

  const ReadSuper getReadSuperFunction() const;

  void setReadSuperFunction(ReadSuper read_super);
};


#endif // FileSystemType_h___


