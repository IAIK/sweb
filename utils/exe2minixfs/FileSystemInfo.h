/**
 * @file FileSystemInfo.h
 */

#ifndef FILESYSTEMINFO_H__
#define FILESYSTEMINFO_H__

#include "MinixFSTypes.h"
#include <string>

class Dentry;
//class VfsMount;



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
   * the current-position-directory
   */
  Dentry* pwd_;

  /**
   * the pathname of a fs_info
   */
  char* pathname_;

 public:

  /**
   * contructor
   */
  FileSystemInfo();

  /**
   * destructor
   */
  ~FileSystemInfo();

  /**
   * set the ROOT-info to the class
   * @param root the root dentry to set
   * @param root_mnt the root_mnt to set
   */
  void setFsRoot(Dentry* root)//, VfsMount* root_mnt)
    { root_ = root; }// root_mnt_ = root_mnt; }

  /**
   * set the PWD-info to the class (PWD: print working directory)
   * @param pwd the current path to set
   * @param pwd_mnt the mount point of the current path to set
   */
  void setFsPwd(Dentry* pwd)//, VfsMount* pwd_mnt)
    { pwd_ = pwd; }// pwd_mnt_ = pwd_mnt; }

  /**
   * get the ROOT-info (ROOT-directory) from the class
   * @return the root dentry
   */
  Dentry* getRoot() { return root_; }

  /**
   * get the PWD-info (PWD-directory) from the class
   * @return the dentry of the current directory
   */
  Dentry* getPwd() { return pwd_; }

  /**
   * read/copy the file pathname of the process
   * @param pathname the name to set
   * @param length the names length
   * @return 0 on success
   */
  int32 setName(const char* pathname, uint32 length = 0);

  /**
   * return the file pathname from the class
   * @return the pathname
   */
  char *getName() { return pathname_; }

  /**
   * release the file pathname
   */
  void putName();
};

#endif // FILESYSTEMINFO_H___
