// Projectname: SWEB
// Simple operating system for educational purposes

#ifndef FILESYSTEMINFO_H___
#define FILESYSTEMINFO_H___

#include "types.h"

class Dentry;
class VfsMount;

//---------------------------------------------------------------------------
/**
 * The information of the file system
 * 
 * This class used to store the system-information (i.e. the root-directory-info,
 * the current-directory-info).
 */

class FileSystemInfo
{
 protected:
  /// the root-directory
  Dentry* root_;
  
  /// the root vfsmount-struct
  VfsMount* root_mnt_;

  /// the current-position-directory
  Dentry* pwd_;
  
  /// the current-position vfsmount-struct
  VfsMount* pwd_mnt_;
  
  /// the alternative-root-directory
  Dentry* alt_root_;
  
  /// the alternative-root vfsmount-struct
  VfsMount* alt_root_mnt_;
  
  /// the pathname of a fs_info
  char* pathname_;
  
 public:

  /// contructor
  FileSystemInfo();
  
  /// destructor
  ~FileSystemInfo() {}
  
  /// set the ROOT-info to the class
  void setFsRoot(Dentry* root, VfsMount* root_mnt)
    { root_ = root; root_mnt_ = root_mnt; }
  
  /// set the PWD-info to the class (PWD: print working directory)
  void setFsPwd(Dentry* pwd, VfsMount* pwd_mnt)
    { pwd_ = pwd; pwd_mnt_ = pwd_mnt; }

  /// set the alternative-info to the class
  // void set_fs_alt_root(Dentry* alt_root, VfsMount* alt_root_mnt)
  // { alt_root_ = alt_root; alt_root_mnt_ = alt_root_mnt; }
  
  /// get the ROOT-info (ROOT-directory) from the class
  Dentry* getRoot() { return root_; }
  
  /// get the ROOT-info (ROOT-VfsMount-info) from the class
  VfsMount* getRootMnt() { return root_mnt_; }
  
  /// get the PWD-info (PWD-directory) from the class
  Dentry* getPwd() { return pwd_; }
  
  /// get the PWD-info (PWD-VfsMount-info) from the class
  VfsMount* getPwdMnt() { return pwd_mnt_; }
  
  /// get the alternative-ROOT-info (alt-ROOT-directory) from the class
  // Dentry* getAltRoot() { return alt_root_; }

  /// get the alternative-ROOT-info (alt-VfsMount-directory) from the class
  // VfsMount* getAltRootMnt() { return alt_root_mnt_; }
  
  /// read/copy the file pathname of the process
  int32 setName(const char* pathname, uint32 length = 0);

  /// return the file pathname from the class
  char* getName() { return pathname_; }
  
  /// release the file pathname
  void putName();
};

#endif // FILESYSTEMINFO_H___
