// Projectname: SWEB
// Simple operating system for educational purposes

#include "fs/FileSystemType.h"
#include "fs/VirtualFileSystem.h"
#include "fs/Dentry.h"
#include "fs/VfsMount.h"
#include "util/string.h"
#include "assert.h"
#include "mm/kmalloc.h"

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
  assert(fs_name);

  uint32 fstl_size = file_system_types_.getLength();

  for (uint32 counter = 0; counter < fstl_size; ++counter) 
  {
    if (strcmp(file_system_types_.at(counter)->getFSName(), fs_name) == 0)
    {
      return file_system_types_.at(counter);
    }
  }

  return 0;

}

//----------------------------------------------------------------------
int32 VirtualFileSystem::root_mount(char* fs_name, int32 /*mode*/)
{
  FileSystemType *fst = getFsType(fs_name);

  Superblock *super = fst->createSuper(0);
  super = fst->readSuper(super, 0);
  Dentry *root = super->getRoot();

  VfsMount *root_mount = new VfsMount(0, 0, root, super, 0);

  mounts_.pushBack(root_mount);
  superblocks_.pushBack(super);

  // fs_info initialize
  fs_info.setFsRoot(root, root_mount);
  fs_info.setFsPwd(root, root_mount);
  
  return 0;
}

//----------------------------------------------------------------------
int32 VirtualFileSystem::rootUmount()
{
  if(superblocks_.getLength() != 1)
  {
    return -1;
  }
  VfsMount *root_vfs_mount = mounts_.at(0);
  delete root_vfs_mount;

  Superblock *root_sb = superblocks_.at(0);
  delete root_sb;
  return 0;
}

//----------------------------------------------------------------------
FileSystemInfo *getFSInfo()
{
  // TODO this needs to be done properly as soon as possible
  
  return &fs_info;
}


