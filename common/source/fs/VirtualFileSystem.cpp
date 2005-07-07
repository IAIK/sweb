
//
// CVS Log Info for $RCSfile: VirtualFileSystem.cpp,v $
//
// $Id: VirtualFileSystem.cpp,v 1.5 2005/07/07 15:01:46 davrieb Exp $
// $Log: VirtualFileSystem.cpp,v $
// Revision 1.4  2005/07/07 12:31:48  davrieb
// add ramfs
//
// Revision 1.3  2005/06/01 09:20:36  davrieb
// add all changes to fs
//
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

FileSystemType::FileSystemType() :
  fs_name_(0),
  fs_flags_(0)
{
}

FileSystemType::~FileSystemType()
{
}

const char* FileSystemType::getFSName() const
{
  return fs_name_;
}

int32 FileSystemType::getFSFlags() const
{
  return fs_flags_;
}

Superblock *FileSystemType::readSuper(int32 /*flags*/, const char* /*dev_name*/)
{
  assert(0);
  return 0;
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

  file_system_types_.pushBack(file_system_type);
  return 0;

}


int32 VirtualFileSystem::unregisterFileSystem(FileSystemType *file_system_type)
{
  assert(file_system_type != 0);

  const char *fs_name = file_system_type-> getFSName();
  uint32 fstl_size = file_system_types_.size();

  for (uint32 counter = 0; counter < fstl_size; ++counter)
  {
    if (strcmp(file_system_types_[counter]->getFSName(), fs_name))
    {
      file_system_types_.remove(counter);
    }
  }

  return 0;
}

//----------------------------------------------------------------------
FileSystemType *VirtualFileSystem::getFsType(const char* fs_name)
{
  assert(fs_name);

  uint32 fstl_size = file_system_types_.size();

  for (uint32 counter = 0; counter < fstl_size; ++counter) {
    if (strcmp(file_system_types_[counter]->getFSName(), fs_name) == 0)
    {
      return file_system_types_[counter];
    }
  }

  return 0;

}

