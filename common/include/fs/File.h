// Projectname: SWEB
// Simple operating system for educational purposes
//
// Copyright (C) 2005  Chen Qiang
// Copyright (C) 2005  David Riebenbauer
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

#ifndef FILE_H__
#define FILE_H__

#include "types.h"
#include "fs/PointList.h"
//#include "Inode.h"
//#include "Dentry.h"

// forward declarations
class Superblock;
class Inode;
class Dentry;

/// The basic flags for files
#define O_RDONLY    0x0000
#define O_WRONLY    0x0001
#define O_RDWR      0x0002

/// The basic access modes for files
#define A_READABLE  0x0001
#define A_WRITABLE  0x0002
#define A_EXECABLE  0x0004

typedef uint32 l_off_t;


class File
{
  public:

    /// Seek from the start of the file.
    static const uint8 SEEK_SET = 0;

    /// Seek from the current position in the file.
    static const uint8 SEEK_CUR = 1;

    /// Seek from the end of the file.
    static const uint8 SEEK_END = 2;

    typedef PointList<File> FList;

    typedef uint32 mode_t;

    class Owner
    {
    };

    /// The user id of the file;
    uint32 uid;

    /// The group id of the file;
    uint32 gid;

    /// interna version number.
    uint32 version;


  protected:

    /// Listof generic file objects
    FList f_list_;

    /// The Filename
    // char *name_;

    /// The superblock  pointing to this file
    Superblock* f_superblock_;
    
    /// The indoe associated to the file.
    Inode* f_inode_;

    /// The dentry pointing to this file/
    Dentry* f_dentry_;

    /// Mounted filesystem containing the file
    //VFSMount *vfs_mount_;

    /// usage counter of the file
    int32 count_;

    /// The flags specified when the file was opened
    uint32 flag_;

    /// The process access mode of the file;
    /// default value: READABLE ^ WRITABLE ^ EXECABLE
    mode_t mode_;

    /// Current offset in the file
    l_off_t offset_;

    /// indicates the owner of the file;
    Owner owner;

  public:

    //----------------------------------------------------------------------
    /// The Constructor
  File(Inode* inode, Dentry* dentry) {f_inode_ = inode; f_dentry_ = dentry;}

    //----------------------------------------------------------------------
    /// The Destructor
  virtual  ~File(){}

    //----------------------------------------------------------------------
    /// Setter method for the file's name.
    ///
    /// @param name is the new name of the file.
    /// @return is an error code or 0 if successfull.
    ///  int32 setName(const char *name);

    //----------------------------------------------------------------------
    /// Getter method for the filename.
    ///
    /// @return is the file's name
  virtual char *getName() const {return 0;}

    //----------------------------------------------------------------------
    /// Getter Method for the dentry.
    ///
    /// @return is the dentry associated to the File.
  virtual Dentry *getDentry() const {return 0;}

    //----------------------------------------------------------------------
    /// Sets the file position relative to the start of the file, the  end of the file or the
    /// current file position.
    ///
    /// @param offset is the offset to set.
    /// @param origin is the on off SEEK_SET, SEEK_CUR and SEEK_END.
    /// @returns the offset from the start off the file or -1 on failure.
    // l_off_t llSeek(l_off_t offset, uint8 origin)

    //----------------------------------------------------------------------
    /// reads from the file
    ///
    /// @param buffer is the buffer where the data is written to
    /// @param count is the number of bytes to read.
    /// @param offset is the offset to read from counted from the start of the file.
  virtual  int32 read(int32 */*buffer*/, size_t /*count*/, l_off_t /*offset*/) {return 0;}

    //----------------------------------------------------------------------
    /// write to the file
    ///
    /// @param buffer is the buffer where the data is read from
    /// @param count is the number of bytes to write.
    /// @param offset is the offset to write from counted from the start of the file.
  virtual  int32 write(int32 */*buffer*/, size_t /*count*/, l_off_t /*offset*/) {return 0;}

    //----------------------------------------------------------------------
    /// Open the file
    ///
    /// @param inode is the inode the read the file from.
  virtual  int32 open(uint32) {return 0;}

  //-----------------------------------------------------------------------
  /// Close the file
  ///
  /// @param inode is close, the superblock has the information, that this
  /// inode is not use anymore.
  virtual  int32 close() {return 0;}

    //----------------------------------------------------------------------
    /// Flush all off the file's write operations. The File will be written to disk.
    ///
    /// @return is the error code of the flush operation.
  virtual  int32 flush() {return 0;}



};


#endif

