#ifndef FILESYSTEMINFO_H___
#define FILESYSTEMINFO_H___

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
  Dentry* current_position_;
  
  /// the current-position vfsmount-struct
  VfsMount* current_position_mnt_;
  
  /// the alternative-root-directory
  Dentry* alt_root_;
  
  /// the alternative-root vfsmount-struct
  VfsMount* alt_root_mnt_;
  
 public:
  FileSystemInfo();
  
  ~FileSystemInfo() {}
  
  void set_fs_root(Dentry* root, VfsMount* root_mnt)
    { root_ = root; root_mnt_ = root_mnt; }
  
  void set_fs_current_position(Dentry* current_position, 
                               VfsMount* current_position_mnt)
    { current_position_ = current_position; 
      current_position_mnt_ = current_position_mnt; }

  // void set_fs_alt_root(Dentry* alt_root, VfsMount* alt_root_mnt)
  // { alt_root_ = alt_root; alt_root_mnt_ = alt_root_mnt; }
  
  Dentry* getRoot() { return root_; }
  VfsMount* getRootMnt() { return root_mnt_; }
  
  Dentry* getCurrentPosition() { return current_position_; }
  VfsMount* getCurrentPositionMnt() { return current_position_mnt_; }
  
  // Dentry* getAltRoot() { return alt_root_; }
  // VfsMount* getAltRootMnt() { return alt_root_mnt_; }
};

#endif // FILESYSTEMINFO_H___
