// Projectname: SWEB
// Simple operating system for educational purposes

#ifndef Inode_h___
#define Inode_h___

#include "types.h"
#include "fs/PointList.h"
#include "console/kprintf.h"

class Dentry;
class File;
class Superblock;

// three possible inode state bits:
#define I_UNUSED 0 // the unused inode state
#define I_DIRTY 1 // Dirty inodes are on the per-super-block s_dirty_ list, and
                  // will be written next time a sync is requested.
#define I_LOCK 2  //state not implemented

// three possible inode type bits:
#define I_FILE         0
#define I_DIR          1
#define I_LNK          2
#define I_CHARDEVICE   3
#define I_BLOCKDEVICE  4

// The per-inode flags:
#define MS_NODEV 2 // If this inode is a device special file, it cannot be
                   // opend.
#define INODE_DEAD 666

//-------------------------------------------------------------------------
/**
 * Inode
 *
 * All information needed by the filesystem to handle a file is included in a
 * data class called an inode.
 */
class Inode
{
 public:
  /// The i_list_ linked list links inodes in various states. There is the
  /// inode_in_use list which lists unchanged inodes that are in active use.
  /// inode_unused which lists unused inodes, and the s_dirty_ of Superblock
  /// class store all the dirty inodes on the given file system.
  // PointList<Inode> i_list_;

  /// The dentry of this inode. (dir)
  Dentry *i_dentry_;

  /// The dentry-PointList of this inode. (file)
  PointList<Dentry> i_dentry_link_;
  
  /// The (open) file of this inode.
  PointList<File> i_files_;

  /// the number of the link of this inode.
  uint32 i_nlink_;

  /// reference of superblock
  Superblock *i_superblock_;

  /// current file size in bytes
  uint32 i_size_;

  /// The basic block size of inode.
  // uint64 i_blksize_;

  /// The power of 2 that i_blksize_ is.
  // uint32 i_blocks_;

  /// There are three possible inode type bits: I_FILE, I_DIR, I_LNK
  uint32 i_type_;

  /// There are three possible inode state bits: I_DIRTY, I_LOCK, I_UNUSED.
  uint32 i_state_;

 public:

  /// contructor
  Inode(Superblock *super_block, uint32 inode_type)
    { i_superblock_ = super_block, i_type_ = inode_type; }

  /// destructor
  virtual ~Inode() {}

  /// Create a directory with the given dentry.
  virtual int32 create(Dentry *) { return 0; }

  /// lookup should check if that name (given by the char-array) exists in the
  /// directory (I_DIR inode) and should return the Dentry if it does.
  /// This involves finding and loading the inode. If the lookup failed to find
  /// anything, this is indicated by returning NULL-pointer.
  virtual Dentry* lookup(const char* /*name*/) {return 0;}

  /// The link method should make a hard link to the name referred to by the
  /// denty, which is in the directory refered to by the Inode. 
  /// (only used for File)
  virtual File* link(uint32 /*flag*/) {return 0;}

  /// This should remove the name refered to by the Dentry from the directory
  /// referred to by the inode. (only used for File)
  virtual int32 unlink(File* /*file*/) {return 0;}

  /// This should create a symbolic link in the given directory with the given
  /// name having the given value. It should d_instantiate the new inode into
  /// the dentry on success.
  virtual int32 symlink(Inode */*inode*/, Dentry */*dentry*/, 
                        const char */*link_name*/) {return 0;}

  /// Create a directory with the given dentry.
  virtual int32 mkdir(Dentry *) {return 0;}

  /// Create a file with the given dentry.
  virtual int32 mkfile(Dentry */*dentry*/) { return 0; }

  /// Remove the named directory (if empty).
  virtual int32 rmdir() {return 0;}
  
  /// REmove the named directory (if empty) or file
  virtual int32 rm() {return 0;}

  /// Create a directory with the given dentry.
  virtual int32 mknod(Dentry *) {return 0;}

  /// change the name to new_name
  virtual int32 rename(const char* /*new_name*/) {return 0;}

  /// The symbolic link referred to by the dentry is read and the value is
  /// copied into the user buffer (with copy_to_user) with a maximum length
  /// given by the intege.
  virtual int32 readlink(Dentry */*dentry*/, char*, int32 /*max_length*/) 
    {return 0;}

  /// If the directory (parent dentry) have a directory and a name within that
  /// directory (child dentry) then the obvious result of following the name
  /// from the directory would arrive at the child dentry. (for symlink)
  /// @param prt_dentry the parent dentry
  /// @param chd_dentry the child dentry
  virtual Dentry* followLink(Dentry */*prt_dentry*/, Dentry */*chd_dentry*/) 
    {return 0;}

  /// read the data from the inode
  virtual int32 readData(uint32 /*offset*/, uint32 /*size*/, char */*buffer*/) 
    {return 0;}

  /// write the data to the inode
  virtual int32 writeData(uint32 /*offset*/, uint32 /*size*/, const char*/*buffer*/) 
    {return 0;}

 public:

  /// insert the opened file point to the file_list of this inode.
  int32 insertOpenedFiles(File*);

  /// remove the opened file point from the file_list of this inode.
  int32 removeOpenedFiles(File*);
  
  /// check the existance of the open-file-list
  bool openedFilesEmpty() { return(i_files_.empty());}
  
  /// return the Superblock where this inode is located

  /// return the Superblock that this inode is located

  Superblock* getSuperblock() { return i_superblock_; }
  
  /// setting the superblock is neccessary because the devices
  /// are created before the DeviceFS is created
  void setSuperBlock( Superblock * sb) {
    i_superblock_ = sb;
  }
  
  /// get the type from inode
  uint32 getType() { return i_type_; }
  
  /// get the pointer of the dentry
  Dentry* getDentry() { return i_dentry_; }
  
  /// get the first file object.
  File* getFirstFile() { return i_files_.at(0); }
  
  uint32 getNumOpenedFile() { return i_files_.getLength(); }
};


#endif // Inode_h___


