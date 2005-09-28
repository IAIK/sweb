#ifndef FILEDESCRIPTOR_H___
#define FILEDESCRIPTOR_H___

#include "types.h"

class File;

class FileDescriptor
{
 protected:
  /// the file descriptor
  uint32 fd_;
  
  /// the file object
  File* file_;
  
 public:
  /// contructor
  FileDescriptor(File* file);
  
  /// destructor
  virtual ~FileDescriptor() {}
  
  /// get the file descriptor
  uint32 getFd() { return fd_; }
  
  /// get the file
  File* getFile() { return file_; }
};

#endif // FILEDESCRIPTOR_H_
