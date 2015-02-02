#ifndef FILESYSTEMINFO_H__
#define FILESYSTEMINFO_H__

#include "types.h"
#include "ustring.h"

class Dentry;
class VfsMount;

/**
 * @class FileSystemInfo The information of the file system
 *
 * This class used to store the system-information (i.e. the root-directory-info,
 * the current-directory-info).
 */
class FileSystemInfo
{
  protected:
    /**
     * the root-directory
     */
    Dentry* root_;

    /**
     * the root vfsmount-struct
     */
    VfsMount* root_mnt_;

    /**
     * the current-position-directory
     */
    Dentry* pwd_;

    /**
     * the current-position vfsmount-struct
     */
    VfsMount* pwd_mnt_;

    /**
     * the alternative-root-directory
     */
    Dentry* alt_root_;

    /**
     * the alternative-root vfsmount-struct
     */
    VfsMount* alt_root_mnt_;

  public:
    FileSystemInfo();
    ~FileSystemInfo();
    FileSystemInfo(const FileSystemInfo& fsi);

    /**
     * set the ROOT-info to the class
     * @param root the root dentry to set
     * @param root_mnt the root_mnt to set
     */
    void setFsRoot(Dentry* root, VfsMount* root_mnt)
    {
      root_ = root;
      root_mnt_ = root_mnt;
    }

    /**
     * set the PWD-info to the class (PWD: print working directory)
     * @param pwd the current path to set
     * @param pwd_mnt the mount point of the current path to set
     */
    void setFsPwd(Dentry* pwd, VfsMount* pwd_mnt)
    {
      pwd_ = pwd;
      pwd_mnt_ = pwd_mnt;
    }

    /**
     * get the ROOT-info (ROOT-directory) from the class
     * @return the root dentry
     */
    Dentry* getRoot()
    {
      return root_;
    }

    /**
     * get the ROOT-info (ROOT-VfsMount-info) from the class
     * @return the VfsMount
     */
    VfsMount* getRootMnt()
    {
      return root_mnt_;
    }

    /**
     * get the PWD-info (PWD-directory) from the class
     * @return the dentry of the current directory
     */
    Dentry* getPwd()
    {
      return pwd_;
    }

    /**
     * get the PWD-info (PWD-VfsMount-info) from the class
     * @return the VfsMount of the current directory
     */
    VfsMount* getPwdMnt()
    {
      return pwd_mnt_;
    }

    ustl::string pathname_;
};

#endif // FILESYSTEMINFO_H___
