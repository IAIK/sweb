
//
// CVS Log Info for $RCSfile: VirtualFileSystem.cpp,v $
//
// $Id: VirtualFileSystem.cpp,v 1.10 2005/09/02 17:57:58 davrieb Exp $
// $Log: VirtualFileSystem.cpp,v $
// Revision 1.9  2005/08/11 16:46:57  davrieb
// add PathWalker
//
// Revision 1.8  2005/07/21 18:07:04  davrieb
// mount of the root directory
//
// Revision 1.7  2005/07/19 17:11:03  davrieb
// put filesystemtype into it's own file
//
// Revision 1.6  2005/07/16 13:22:00  davrieb
// rrename List in fs to PointList to avoid name clashes
//
// Revision 1.5  2005/07/07 15:01:46  davrieb
// a few vfs fixes
//
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

#include "fs/FileSystemType.h"
#include "fs/VirtualFileSystem.h"
#include "fs/Dentry.h"
#include "fs/VfsMount.h"
#include "util/string.h"
#include "assert.h"

#include "fs/fs_global.h"



/// Global VirtualFileSystem object
VirtualFileSystem vfs;




//----------------------------------------------------------------------
VirtualFileSystem::VirtualFileSystem()
{
}

//----------------------------------------------------------------------
VirtualFileSystem::~VirtualFileSystem()
{
}

//----------------------------------------------------------------------
int32 VirtualFileSystem::registerFileSystem(FileSystemType *file_system_type)
{
  assert(file_system_type);
  assert(file_system_type->getFSName());

  file_system_types_.pushBack(file_system_type);
  return 0;

}

//----------------------------------------------------------------------
int32 VirtualFileSystem::unregisterFileSystem(FileSystemType *file_system_type)
{
  assert(file_system_type != 0);

  const char *fs_name = file_system_type-> getFSName();
  uint32 fstl_size = file_system_types_.size();

  for (uint32 counter = 0; counter < fstl_size; ++counter)
  {
    if (strcmp(file_system_types_[counter]->getFSName(), fs_name))
    {
      delete file_system_types_[counter];
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

//----------------------------------------------------------------------
int32 VirtualFileSystem::root_mount(char* fs_name, int32 mode)
{
  FileSystemType *fst = getFsType(fs_name);


  Superblock *super = fst->createSuper(0);
  super = fst->readSuper(super, 0);
  Dentry *root = super->s_root_;

  VfsMount *root_mount = new VfsMount(0, 0, root, super, 0);

  mounts_.pushBack(root_mount);
  superblocks_.push_end(super);
}

//----------------------------------------------------------------------
FileSystemInfo *getFSInfo()
{
  // TODO this needs to be done properly as soon as possible
  return &fs_info;
}


