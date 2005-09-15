// Projectname: SWEB
// Simple operating system for educational purposes

#ifndef VFS_SYSCALL_H___
#define VFS_SYSCALL_H___

#include "types.h"

class Dirent;

class VfsSyscall
{
 protected:
  /// checks the duplication from the pathname in the file-system
  ///
  ///@param pathname the input pathname
  ///@return On success, zero is returned. On error, -1 is returned.
  int32 dupChecking(const char* pathname);

 public:
  VfsSyscall() {}
  
  virtual ~VfsSyscall() {}
 
  /// make a new directory.
  /// i.e. im the path "/file/test/" create a new directory with the name
  /// "dir". => the new_dir ist "/file/test/dir"
  ///
  ///@param pathname the new directory.
  ///@param mode the permission. 
  ///@return On success, zero is returned. On error, -1 is returned.
  virtual int32 mkdir(const char* pathname, int32 /*mode*/);
  
  /// The readdir() display the names from all childs and returns a pointer 
  /// to a Dirent.
  ///
  ///@param pathname the destination-directory.
  virtual Dirent* readdir(const char* pathname);
  
  /// chdir() changes the current directory to the specified directory.
  ///
  ///@param dir the destination-directory.
  ///@return On success, zero is returned. On error, -1 is returned.
  virtual int32 chdir(const char* pathname);
  
  /// delete a directory, which must be empty.
  ///
  ///@param dir the removed directory
  ///@return On success, zero is returned. On error, -1 is returned.
  virtual int32 rmdir(const char* pathname);
};

#endif // VFS_SYSCALL_H___
