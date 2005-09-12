#ifndef FILESYSTEMINFO_H___
#define FILESYSTEMINFO_H___

#include "types.h"

class Dentry;
class VfsMount;

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
  FileSystemInfo();
  
  ~FileSystemInfo() {}
  
  void setFsRoot(Dentry* root, VfsMount* root_mnt)
    { root_ = root; root_mnt_ = root_mnt; }
  
  void setFsPwd(Dentry* pwd, VfsMount* pwd_mnt)
    { pwd_ = pwd; pwd_mnt_ = pwd_mnt; }

  // void set_fs_alt_root(Dentry* alt_root, VfsMount* alt_root_mnt)
  // { alt_root_ = alt_root; alt_root_mnt_ = alt_root_mnt; }
  
  Dentry* getRoot() { return root_; }
  VfsMount* getRootMnt() { return root_mnt_; }
  
  Dentry* getPwd() { return pwd_; }
  VfsMount* getPwdMnt() { return pwd_mnt_; }
  
  // Dentry* getAltRoot() { return alt_root_; }
  // VfsMount* getAltRootMnt() { return alt_root_mnt_; }
  
  /// read the file pathname of the process
  int32 setName(char* pathname, uint32 length = 0);
  
  char* getName() { return pathname_; }
  
  /// release the pathname_
  void putName();
};

#endif // FILESYSTEMINFO_H___
