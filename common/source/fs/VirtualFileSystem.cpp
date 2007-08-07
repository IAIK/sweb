// Projectname: SWEB
// Simple operating system for educational purposes

#include "fs/FileSystemType.h"
#include "fs/VirtualFileSystem.h"
#include "fs/Dentry.h"
#include "fs/Superblock.h"
#include "fs/VfsMount.h"
#include "util/string.h"
#include "assert.h"
#include "mm/kmalloc.h"
#include "arch_bd_manager.h"

#include "console/kprintf.h"
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
    if(strcmp(file_system_types_.at(counter)->getFSName(), fs_name) == 0)
    {
      FileSystemType *fst = file_system_types_.at(counter);
      delete fst;
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
VfsMount *VirtualFileSystem::getVfsMount(const Dentry* dentry, bool is_root)
{
  assert(dentry);

  uint32 vfs_mount_size = mounts_.getLength();
  kprintfd( "getVfsMount> vfs_mount_size : %d\n",vfs_mount_size);

  if(is_root == false)
  {
    for (uint32 counter = 0; counter < vfs_mount_size; ++counter)
    {
      kprintfd( "getVfsMount> mounts_.at(counter)->getMountPoint()->getName() : %s\n",mounts_.at(counter)->getMountPoint()->getName());
      if((mounts_.at(counter)->getMountPoint()) == dentry)
      {
        return mounts_.at(counter);
      }
    }
  }
  else
  {
    for (uint32 counter = 0; counter < vfs_mount_size; ++counter)
    {
      if((mounts_.at(counter)->getRoot()) == dentry)
      {
        return mounts_.at(counter);
      }
    }
  }

  return 0;

}

//----------------------------------------------------------------------
FileSystemInfo *VirtualFileSystem::root_mount(char* fs_name, uint32 /*flags*/)
{
  FileSystemType *fst = getFsType(fs_name);

  Superblock *super = fst->createSuper(0,0);
  super = fst->readSuper(super, 0);
  Dentry *mount_point = super->getMountPoint();
  mount_point->setMountPoint( mount_point );
  Dentry *root = super->getRoot();

  VfsMount *root_mount = new VfsMount(0, mount_point, root, super, 0);

  mounts_.pushBack(root_mount);
  superblocks_.pushBack(super);

  // fs_info initialize
  FileSystemInfo *fs_info = new FileSystemInfo();
  fs_info->setFsRoot(root, root_mount);
  fs_info->setFsPwd(root, root_mount);
  
  return fs_info;
}

//----------------------------------------------------------------------
int32 VirtualFileSystem::mount(const char* dev_name, const char* dir_name,
                               char* fs_name, uint32 /*flags*/)
{
  FileSystemInfo *fs_info = currentThread->getFSInfo();
  if(!dev_name)
    return -1;
  if((!dir_name) || (!fs_name))
    return -1;

  uint32 dev = BDManager::getInstance()->getDeviceByName(dev_name)->getDeviceNumber();
  kprintfd("dev_nr:%d", dev);
  FileSystemType *fst = getFsType(fs_name);
  
  fs_info->setName(dir_name);
  char* test_name = fs_info->getName();

  int32 success = path_walker.pathInit(test_name, 0);
  if(success == 0)
    success = path_walker.pathWalk(test_name);
  fs_info->putName();
  
  if(success != 0)
    return -1;
  
  // found the mount point
  Dentry *found_dentry = path_walker.getDentry();
  VfsMount *found_vfs_mount = path_walker.getVfsMount();

  // create a new superblock
  Superblock *super = fst->createSuper(found_dentry, dev);
  super = fst->readSuper(super, 0); //?
  Dentry *root = super->getRoot();
  
  // create a new vfs_mount
  VfsMount *std_mount = new VfsMount(found_vfs_mount, found_dentry,
                                     root, super, 0);
  mounts_.pushBack(std_mount);
  superblocks_.pushBack(super);
  return 0;
}

//----------------------------------------------------------------------
int32 VirtualFileSystem::rootUmount()
{
  if(superblocks_.getLength() == 0)
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
int32 VirtualFileSystem::umount(const char* dir_name, uint32 /*flags*/)
{
  FileSystemInfo *fs_info = currentThread->getFSInfo();
  if(dir_name == 0)
    return -1;

  fs_info->setName(dir_name);
  char* test_name = fs_info->getName();

  int32 success = path_walker.pathInit(test_name, 0);
  if(success == 0)
    success = path_walker.pathWalk(test_name);
  fs_info->putName();
  
  if(success != 0)
    return -1;
  
  // test the umount point\n
  Dentry *found_dentry = path_walker.getDentry();
  VfsMount * found_vfs_mount = path_walker.getVfsMount();

  if(found_vfs_mount == 0)
  {
    kprintfd("umount point error\n");
    return -1;
  }
  else
    kprintfd("umount point found\n");
  
  // in the case, the current-directory is in the local-root of the umounted
  // filesystem
  if(fs_info->getPwdMnt() == found_vfs_mount)
  {
    if(fs_info->getPwd() == found_dentry)
    {
      kprintfd("the mount point exchange\n");
      fs_info->setFsPwd(found_vfs_mount->getMountPoint(),
                       found_vfs_mount->getParent());
    }
    else
    {
      kprintfd("set PWD NULL\n");
      fs_info->setFsPwd(0,0);
    }
  }
  
  Superblock *sb = found_vfs_mount->getSuperblock();
  delete found_vfs_mount;
  delete sb;
  
  return 0;
}

//----------------------------------------------------------------------
FileSystemInfo *getFSInfo()
{
  // TODO this needs to be done properly as soon as possible
  
  return currentThread->getFSInfo();
}


