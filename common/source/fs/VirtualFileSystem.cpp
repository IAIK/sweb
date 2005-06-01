
//
// CVS Log Info for $RCSfile: VirtualFileSystem.cpp,v $
//
// $Id: VirtualFileSystem.cpp,v 1.3 2005/06/01 09:20:36 davrieb Exp $
// $Log: VirtualFileSystem.cpp,v $
// Revision 1.2  2005/05/31 20:25:28  btittelbach
// moved assert to where it belongs (arch) and created nicer version
//
// Revision 1.1  2005/05/10 16:42:30  davrieb
// add first attempt to write a virtual file system
//
//

#include "fs/VirtualFileSystem.h"
#include "util/string.h"
#include "assert.h"

FileSystemType::FileSystemType()
{
  fs_name_ = 0;
}

FileSystemType::FileSystemType(const char* fs_name) :
    fs_name_(fs_name)
{
  assert(fs_name != 0);
}

FileSystemType::~FileSystemType()
{
}

const char* FileSystemType::getFSName() const
{
  return fs_name_;
}

void FileSystemType::setFSName(const char* fs_name)
{
  assert(fs_name != 0);

  fs_name_ = fs_name;
}

int32 FileSystemType::getFSFlags() const
{
  return fs_flags_;
}

void FileSystemType::setFSFlags(int32 fs_flags)
{
  fs_flags_ = fs_flags;
}

const FileSystemType::ReadSuper FileSystemType::getReadSuperFunction() const
{
  return read_super_;
}

void FileSystemType::setReadSuperFunction(FileSystemType::ReadSuper read_super)
{
  read_super_ = read_super;
}

VirtualFileSystem::VirtualFileSystem():
  superblock_(0)
{
}

VirtualFileSystem::~VirtualFileSystem()
{
}

int32 VirtualFileSystem::registerFileSystem(FileSystemType *file_system_type)
{
  if (file_system_type == 0)
  {
    return -1;
  }

  if (file_system_type->getFSName() == 0)
  {
    return -2;
  }

  uint32 fst_counter = 0;

  while ((fst_counter < MAX_FILE_SYSTEM_TYPES))
  {
    if (file_system_types_ + fst_counter)
    {
      if (strcmp(file_system_types_[fst_counter].getFSName(), file_system_type->getFSName()))
      {
        return -1;
      }
      ++fst_counter;
    }

    // TODO
    // enter new entry in the rigth way
    FileSystemType* entry = (file_system_types_ + fst_counter);
    entry = file_system_type;
    break;
  }

  return 0;
}


int32 VirtualFileSystem::unregisterFileSystem(FileSystemType *file_system_type)
{
  assert(file_system_type != 0);

  // const char *fs_name = file_system_type-> getFSName();

  for (int32 count = 0; count < MAX_FILE_SYSTEM_TYPES; ++count)
  {
  }

  return 0;
}
