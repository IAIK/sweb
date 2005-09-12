#ifndef VFS_SYSCALL_H___
#define VFS_SYSCALL_H___

#include "types.h"

class Dirent;

class VfsSyscall
{
 public:
  VfsSyscall() {}
  
  virtual ~VfsSyscall() {}
 
  /// make a new directory.
  /// i.e. im the path "/file/test/" create a new directory with the name
  /// "dir". => the new_dir ist "/file/test/dir"
  ///
  ///@param new_dir the new directory.
  ///@param mode the permission. 
  ///@return On success, zero is returned. On error, -1 is returned.
  virtual int32 mkdir(char* new_dir, int32 /*mode*/);
  
  /// The readdir() display the names from all childs and returns a pointer 
  /// to a Dirent.
  ///
  ///@param dir the destination-directory.
  virtual Dirent* readdir(char* dir);
  
  /// chdir() changes the current directory to the specified directory.
  ///
  ///@param dir the destination-directory.
  ///@return On success, zero is returned. On error, -1 is returned.
  virtual int32 chdir(char* dir);
  
  /// delete a directory, which must be empty.
  ///
  ///@param dir the removed directory
  ///@return On success, zero is returned. On error, -1 is returned.
  virtual int32 rmdir(char* dir);
};

#endif // VFS_SYSCALL_H___
