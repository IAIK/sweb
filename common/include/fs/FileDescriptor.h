#pragma once

#include "Mutex.h"

#include "types.h"

#include "EASTL/list.h"
#include "EASTL/map.h"

class File;
class FileDescriptor;
class FileDescriptorList;

class FileDescriptor
{
  protected:
    size_t fd_;
    File* file_;

  public:
    FileDescriptor(File* file);
    virtual ~FileDescriptor();
    [[nodiscard]] uint32 getFd() const { return fd_; }
    [[nodiscard]] File* getFile() const { return file_; }

    friend File;
};

class FileDescriptorList
{
public:
    FileDescriptorList();
    ~FileDescriptorList();

    int add(FileDescriptor* fd);
    int remove(FileDescriptor* fd);
    [[nodiscard]] FileDescriptor* getFileDescriptor(uint32 fd);

    static FileDescriptorList& globalFdList();

private:
    eastl::list<FileDescriptor*> fds_;
    Mutex fd_lock_;
};
