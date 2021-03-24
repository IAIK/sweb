#include "FileSystemType.h"
#include "FileSystemInfo.h"
#include "PathWalker.h"
#include "VirtualFileSystem.h"
#include "Dentry.h"
#include "Superblock.h"
#include "VfsMount.h"
#include "kstring.h"
#include "assert.h"
#include "BDManager.h"
#include "BDVirtualDevice.h"
#include "Thread.h"

#include "console/kprintf.h"

VirtualFileSystem vfs;

void VirtualFileSystem::initialize()
{
  new (this) VirtualFileSystem();
}

VirtualFileSystem::VirtualFileSystem()
{
}

VirtualFileSystem::~VirtualFileSystem()
{
}

int32 VirtualFileSystem::registerFileSystem(FileSystemType *file_system_type)
{
  assert(file_system_type);
  assert(file_system_type->getFSName());

  // check whether a file system type with that name has already been
  // registered
  if (getFsType(file_system_type->getFSName()))
    return -1;
  file_system_types_.push_back(file_system_type);
  return 0;
}

int32 VirtualFileSystem::unregisterFileSystem(FileSystemType *file_system_type)
{
  assert(file_system_type != 0);

  const char *fs_name = file_system_type->getFSName();
  for (FileSystemType* fst : file_system_types_)
  {
    if (strcmp(fst->getFSName(), fs_name) == 0)
      delete fst;
  }
  return 0;
}

FileSystemType *VirtualFileSystem::getFsType(const char* fs_name)
{
  assert(fs_name);

  for (FileSystemType* fst : file_system_types_)
  {
    if (strcmp(fst->getFSName(), fs_name) == 0)
      return fst;
  }
  return 0;
}

VfsMount *VirtualFileSystem::getVfsMount(const Dentry* dentry, bool is_root)
{
  assert(dentry);

  if (is_root == false)
  {
    for (VfsMount* mnt : mounts_)
    {
      debug(VFS, "getVfsMount> mnt->getMountPoint()->getName() : %s\n", mnt->getMountPoint()->getName());
      if (!is_root && (mnt->getMountPoint()) == dentry)
        return mnt;
      else if (mnt->getRoot() == dentry)
        return mnt;
    }
  }
  return 0;
}

FileSystemInfo *VirtualFileSystem::rootMount(const char *fs_name, uint32 /*flags*/)
{
  FileSystemType *fst = getFsType(fs_name);
  if(!fst)
  {
      debug(VFS, "Unknown file system %s\n", fs_name);
      return nullptr;
  }

  if(fst->getFSFlags() & FS_REQUIRES_DEV)
  {
      debug(VFS, "Only file systems that do not require a device are currently supported as root file system\n");
      return nullptr;
  }

  debug(VFS, "Create root %s superblock\n", fst->getFSName());
  Superblock *super = fst->createSuper(-1);
  super = fst->readSuper(super, 0);

  Dentry *root = super->getRoot();
  super->setMountPoint(root);
  Dentry *mount_point = super->getMountPoint();
  mount_point->setMountPoint(mount_point);

  VfsMount *root_mount = new VfsMount(0, mount_point, root, super, 0);

  mounts_.push_back(root_mount);
  superblocks_.push_back(super);

  // fs_info initialize
  FileSystemInfo *fs_info = new FileSystemInfo();
  fs_info->setFsRoot(root, root_mount);
  fs_info->setFsPwd(root, root_mount);

  return fs_info;
}

int32 VirtualFileSystem::mount(const char* dev_name, const char* dir_name, const char* fs_name, uint32 /*flags*/)
{
  debug(VFS, "Mount fs %s at %s with device %s\n", fs_name, dir_name, dev_name ? dev_name : "(null)");

  if ((!dir_name) || (!fs_name))
      return -1;

  FileSystemType *fst = getFsType(fs_name);
  if (!fst)
      return -1;

  if ((fst->getFSFlags() & FS_REQUIRES_DEV) && !dev_name)
      return -1;

  FileSystemInfo *fs_info = currentThread->getWorkingDirInfo();
  assert(fs_info);

  uint32_t dev = -1;
  if(fst->getFSFlags() & FS_REQUIRES_DEV)
  {
      BDVirtualDevice* bddev = BDManager::getInstance()->getDeviceByName(dev_name);
      if (!bddev)
      {
          debug(VFS, "mount: device with name %s doesn't exist\n", dev_name);
          return -1;
      }

      dev = bddev->getDeviceNumber();
      debug(VFS, "mount: dev_nr: %d\n", dev);
  }

  // Find mount point
  fs_info->pathname_ = dir_name;
  Dentry* mountpoint_dentry = 0;
  VfsMount* mountpoint_vfs_mount = 0;
  int32 success = PathWalker::pathWalk(fs_info->pathname_.c_str(), 0, mountpoint_dentry, mountpoint_vfs_mount);

  if (success != 0)
  {
      debug(VFS, "mount: Could not find mountpoint\n");
      return -1;
  }

  // create a new superblock
  debug(VFS, "Create %s superblock\n", fst->getFSName());
  Superblock *super = fst->createSuper(dev);
  if (!super)
  {
      debug(VFS, "mount: Superblock creation failed\n");
      return -1;
  }

  debug(VFS, "mount: Fill superblock\n");
  super = fst->readSuper(super, 0); //?

  Dentry *root = super->getRoot();

  super->setMountPoint(mountpoint_dentry);
  mountpoint_dentry->setMountPoint(root);

  // create a new vfs_mount
  VfsMount *std_mount = new VfsMount(mountpoint_vfs_mount, mountpoint_dentry, root, super, 0);
  mounts_.push_back(std_mount);
  superblocks_.push_back(super);
  return 0;
}

int32 VirtualFileSystem::rootUmount()
{
  if (superblocks_.size() == 0)
  {
    return -1;
  }
  VfsMount *root_vfs_mount = mounts_.at(0);
  delete root_vfs_mount;

  Superblock *root_sb = superblocks_.at(0);
  delete root_sb;
  return 0;
}

int32 VirtualFileSystem::umount(const char* dir_name, uint32 /*flags*/)
{
  FileSystemInfo *fs_info = currentThread->getWorkingDirInfo();
  if (dir_name == 0)
    return -1;

  fs_info->pathname_ = dir_name;
  Dentry* mountpoint_dentry = 0;
  VfsMount* mountpoint_vfs_mount = 0;
  int32 success = PathWalker::pathWalk(fs_info->pathname_.c_str(), 0, mountpoint_dentry, mountpoint_vfs_mount);

  if (success != 0)
    return -1;

  if (mountpoint_vfs_mount == 0)
  {
    debug(VFS, "umount point error\n");
    return -1;
  }
  else
    debug(VFS, "umount point found\n");

  // in the case, the current-directory is in the local-root of the umounted
  // filesystem
  if (fs_info->getPwdMnt() == mountpoint_vfs_mount)
  {
    if (fs_info->getPwd() == mountpoint_dentry)
    {
      debug(VFS, "the mount point exchange\n");
      fs_info->setFsPwd(mountpoint_vfs_mount->getMountPoint(), mountpoint_vfs_mount->getParent());
    }
    else
    {
      debug(VFS, "set PWD NULL\n");
      fs_info->setFsPwd(0, 0);
    }
  }

  Superblock *sb = mountpoint_vfs_mount->getSuperblock();

  mounts_.remove(mountpoint_vfs_mount);
  delete mountpoint_vfs_mount;
  delete sb;

  return 0;
}

