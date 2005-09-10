
//
// CVS Log Info for $RCSfile: VirtualFileSystem.cpp,v $
//
// $Id: VirtualFileSystem.cpp,v 1.11 2005/09/10 19:25:27 qiangchen Exp $
// $Log: VirtualFileSystem.cpp,v $
// Revision 1.10  2005/09/02 17:57:58  davrieb
// preparations to  build a standalone filesystem testsuite
//
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

#include "console/kprintf.h"

/// Global VirtualFileSystem object
VirtualFileSystem vfs;




//----------------------------------------------------------------------
VirtualFileSystem::VirtualFileSystem()
{
  kprintfd("***** contructor of VFS\n");
}

//----------------------------------------------------------------------
VirtualFileSystem::~VirtualFileSystem()
{
}

//----------------------------------------------------------------------
int32 VirtualFileSystem::registerFileSystem(FileSystemType *file_system_type)
{
//  kprintfd("register_test1\n");
  assert(file_system_type);
  assert(file_system_type->getFSName());
//  kprintfd("register_test2\n");
  file_system_types_.pushBack(file_system_type);
//  kprintfd("register_test3\n");
    return 0;

}

//----------------------------------------------------------------------
int32 VirtualFileSystem::unregisterFileSystem(FileSystemType *file_system_type)
{
  assert(file_system_type != 0);

  const char *fs_name = file_system_type-> getFSName();
  uint32 fstl_size = file_system_types_.getLength();

  for (uint32 counter = 0; counter < fstl_size; ++counter)
  {
    if (strcmp(file_system_types_[counter]->getFSName(), fs_name) == 0)
    {
      delete file_system_types_[counter];
      file_system_types_.remove(file_system_types_[counter]);
    }
  }

  return 0;
}

//----------------------------------------------------------------------
FileSystemType *VirtualFileSystem::getFsType(const char* fs_name)
{
  kprintfd("getFsType test\n");
  assert(fs_name);

  uint32 fstl_size = file_system_types_.getLength();

  for (uint32 counter = 0; counter < fstl_size; ++counter) 
  {
//    if(strcmp(file_system_types_.at(counter)->getFSName(), fs_name) == 0)    
    if (strcmp(file_system_types_.at(counter)->getFSName(), fs_name) == 0)
    {
      kprintfd("find the fsType\n");
//      return file_system_types_[counter];
      return file_system_types_.at(counter);
    }
  }
  kprintfd("do not find the fsType\n");

  return 0;

}

//----------------------------------------------------------------------
int32 VirtualFileSystem::root_mount(char* fs_name, int32 /*mode*/)
{
  kprintfd("root_mount_test1\n");
  FileSystemType *fst = getFsType(fs_name);

  kprintfd("root_mount_test2\n");
  Superblock *super = fst->createSuper(0);
  kprintfd("root_mount_test3\n");
  super = fst->readSuper(super, 0);
  kprintfd("root_mount_test4\n");
  Dentry *root = super->getRoot();

  kprintfd("root_mount_test5\n");
  VfsMount *root_mount = new VfsMount(0, 0, root, super, 0);

  kprintfd("root_mount_test6\n");
  mounts_.pushBack(root_mount);
  kprintfd("root_mount_test7\n");
  superblocks_.pushBack(super);
  kprintfd("root_mount_test8\n"); 

  // fs_info initialize
  kprintfd("begin set the fs root info\n");
  fs_info.setFsRoot(root, root_mount);
  fs_info.setFsPwd(root, root_mount);
  kprintfd("end set the fs root info\n");
  return 0;
}

//----------------------------------------------------------------------
FileSystemInfo *getFSInfo()
{
  // TODO this needs to be done properly as soon as possible
  
  return &fs_info;
}


