#pragma once

#include "types.h"
#include "ulist.h"

class File;
class FileDescriptor;

extern ustl::list<FileDescriptor*> global_fd;

class FileDescriptor
{
  protected:
    size_t fd_;
    File* file_;

  public:
    FileDescriptor ( File* file );
    virtual ~FileDescriptor() {}
    uint32 getFd() { return fd_; }
    File* getFile() { return file_; }

    static void add(FileDescriptor* fd);
    static void remove(FileDescriptor* fd);
};

