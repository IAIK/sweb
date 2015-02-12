#include "VfsSyscall.h"
#include "kstring.h"
#include "assert.h"
#include "Dirent.h"
#include "Thread.h"
#include "Mutex.h"
#include "Inode.h"
#include "Dentry.h"
#include "Superblock.h"
#include "File.h"
#include "FileDescriptor.h"
#include "FileSystemType.h"
#include "VirtualFileSystem.h"
#include "MinixFSType.h"
#include "PathWalker.h"
#include "VfsMount.h"
#include "kprintf.h"

#define SEPARATOR '/'
#define CHAR_DOT '.'

extern FileSystemInfo* default_working_dir;

FileDescriptor* VfsSyscall::getFileDescriptor(uint32 fd)
{
  extern Mutex global_fd_lock;
  MutexLock mlock(global_fd_lock);
  for (auto it : global_fd)
  {
    if (it->getFd() == fd)
    {
      debug(VFSSYSCALL, "found the fd\n");
      return it;
    }
  }
  return 0;
}

int32 VfsSyscall::dupChecking(const char* pathname, Dentry*& pw_dentry, VfsMount*& pw_vfs_mount)
{
  FileSystemInfo *fs_info = currentThread ? currentThread->getWorkingDirInfo() : default_working_dir;
  if (pathname == 0)
    return -1;

  bool prepend_slash_dot = true;
  uint32 len = strlen(pathname);

  if (len > 0 && pathname[0] == SEPARATOR)
    prepend_slash_dot = false;
  else if (pathname[0] == CHAR_DOT)
  {
    if (len > 1 && pathname[1] == SEPARATOR)
      prepend_slash_dot = false;
    else if (pathname[1] == CHAR_DOT)
    {
      if (len > 2 && pathname[2] == SEPARATOR)
        prepend_slash_dot = false;
    }
  }

  if (prepend_slash_dot)
  {
    fs_info->pathname_ = "./";
  }
  else
    fs_info->pathname_ = "";
  fs_info->pathname_ += pathname;

  return PathWalker::pathWalk(fs_info->pathname_.c_str(), 0, pw_dentry, pw_vfs_mount);
}

int32 VfsSyscall::mkdir(const char* pathname, int32)
{
  debug(VFSSYSCALL, "(mkdir) \n");
  FileSystemInfo *fs_info = currentThread->getWorkingDirInfo();
  Dentry* pw_dentry = 0;
  VfsMount* pw_vfs_mount = 0;
  if (dupChecking(pathname, pw_dentry, pw_vfs_mount) == 0)
  {
    debug(VFSSYSCALL, "(mkdir) the pathname exists\n");
    return -1;
  }
  uint32 len = fs_info->pathname_.find_last_of("/");
  ustl::string sub_dentry_name = fs_info->pathname_.substr(len + 2, fs_info->pathname_.length() - len - 1);
  // set directory
  fs_info->pathname_ = fs_info->pathname_.substr(0, len + 2);

  debug(VFSSYSCALL, "(mkdir) path_prev_name: %s\n", fs_info->pathname_.c_str());
  pw_dentry = 0;
  pw_vfs_mount = 0;
  int32 success = PathWalker::pathWalk(fs_info->pathname_.c_str(), 0, pw_dentry, pw_vfs_mount);

  if (success != 0)
  {
    debug(VFSSYSCALL, "path_walker failed\n\n");
    return -1;
  }

  Inode* current_inode = pw_dentry->getInode();
  Superblock* current_sb = current_inode->getSuperblock();

  if (current_inode->getType() != I_DIR)
  {
    debug(VFSSYSCALL, "This path is not a directory\n\n");
    return -1;
  }

  // create a new dentry
  Dentry *sub_dentry = new Dentry(pw_dentry);
  sub_dentry->d_name_ = sub_dentry_name;
  debug(VFSSYSCALL, "(mkdir) creating Inode: current_dentry->getName(): %s\n", pw_dentry->getName());
  debug(VFSSYSCALL, "(mkdir) creating Inode: sub_dentry->getName(): %s\n", sub_dentry->getName());
  debug(VFSSYSCALL, "(mkdir) current_sb: %d\n", current_sb);
  debug(VFSSYSCALL, "(mkdir) current_sb->getFSType(): %d\n", current_sb->getFSType());

  current_sb->createInode(sub_dentry, I_DIR);
  debug(VFSSYSCALL, "(mkdir) sub_dentry->getInode(): %d\n", sub_dentry->getInode());
  return 0;
}

Dirent* VfsSyscall::readdir(const char* pathname)
{
  FileSystemInfo *fs_info = currentThread ? currentThread->getWorkingDirInfo() : default_working_dir;
  Dentry* pw_dentry = 0;
  VfsMount* pw_vfs_mount = 0;
  if (dupChecking(pathname, pw_dentry, pw_vfs_mount) == 0)
  {
    Dentry* pw_dentry = 0;
    VfsMount* pw_vfs_mount = 0;
    int32 success = PathWalker::pathWalk(fs_info->pathname_.c_str(), 0, pw_dentry, pw_vfs_mount);

    if (success != 0)
    {
      debug(VFSSYSCALL, "(list) path_walker failed\n\n");
      return ((Dirent*) 0);
    }

    if (pw_dentry->getInode()->getType() != I_DIR)
    {
      debug(VFSSYSCALL, "This path is not a directory\n\n");
      return (Dirent*) 0;
    }

    debug(VFSSYSCALL, "listing dir %s:\n", pw_dentry->getName());
    for (Dentry* sub_dentry : pw_dentry->d_child_)
    {
      Inode* sub_inode = sub_dentry->getInode();
      uint32 inode_type = sub_inode->getType();
      switch (inode_type)
      {
        case I_DIR:
          kprintf("[D] ");
          break;
        case I_FILE:
          kprintf("[F] ");
          break;
        case I_LNK:
          kprintf("[L] ");
          break;
        default:
          break;
      }
      kprintf("%s\n", sub_dentry->getName());
    }
  }
  else
  {
    debug(VFSSYSCALL, "(list) Path doesn't exist\n");
  }
  return 0;
}

int32 VfsSyscall::chdir(const char* pathname)
{
  FileSystemInfo *fs_info = currentThread->getWorkingDirInfo();
  Dentry* pw_dentry = 0;
  VfsMount* pw_vfs_mount = 0;
  if (dupChecking(pathname, pw_dentry, pw_vfs_mount) != 0)
  {
    debug(VFSSYSCALL, "Error: (chdir) the directory does not exist.\n");
    return -1;
  }

  Inode* current_inode = pw_dentry->getInode();
  if (current_inode->getType() != I_DIR)
  {
    debug(VFSSYSCALL, "This path is not a directory\n\n");
    return -1;
  }

  fs_info->setFsPwd(pw_dentry, pw_vfs_mount);

  return 0;
}

int32 VfsSyscall::rm(const char* pathname)
{
  debug(VFSSYSCALL, "(rm) name: %s\n", pathname);
  Dentry* pw_dentry = 0;
  VfsMount* pw_vfs_mount = 0;
  if (dupChecking(pathname, pw_dentry, pw_vfs_mount) != 0)
  {
    debug(VFSSYSCALL, "Error: (rm) the directory does not exist.\n");
    return -1;
  }
  debug(VFSSYSCALL, "(rm) current_dentry->getName(): %s \n", pw_dentry->getName());
  Inode* current_inode = pw_dentry->getInode();
  debug(VFSSYSCALL, "(rm) current_inode: %d\n", current_inode);

  if (current_inode->getType() != I_FILE)
  {
    debug(VFSSYSCALL, "This is not a file\n");
    return -1;
  }

  Superblock* sb = current_inode->getSuperblock();
  if (current_inode->rm() == INODE_DEAD)
  {
    debug(VFSSYSCALL, "remove the inode %d from the list of sb: %d\n", current_inode, sb);
    sb->delete_inode(current_inode);
    debug(VFSSYSCALL, "removed\n");
  }
  else
  {
    debug(VFSSYSCALL, "remove the inode failed\n");
    return -1;
  }

  return 0;
}

int32 VfsSyscall::rmdir(const char* pathname)
{
  Dentry* pw_dentry = 0;
  VfsMount* pw_vfs_mount = 0;
  if (dupChecking(pathname, pw_dentry, pw_vfs_mount) != 0)
  {
    debug(VFSSYSCALL, "Error: (rmdir) the directory does not exist.\n");
    return -1;
  }

  Dentry* current_dentry = pw_dentry;
  Inode* current_inode = current_dentry->getInode();

  //if directory is read from a real file system,
  //it can't ever be empty, "." and ".." are still
  //contained; this has to be checked in Inode::rmdir()

  if (current_inode->getType() != I_DIR)
  {
    debug(VFSSYSCALL, "This is not a directory\n");
    return -1;
  }

  Superblock* sb = current_inode->getSuperblock();
  if (current_inode->rmdir() == INODE_DEAD)
  {
    debug(VFSSYSCALL, "remove the inode from the list\n");
    sb->delete_inode(current_inode);
  }
  else
  {
    debug(VFSSYSCALL, "remove the inode failed\n");
    return -1;
  }

  return 0;
}

int32 VfsSyscall::close(uint32 fd)
{
  FileDescriptor* file_descriptor = getFileDescriptor(fd);

  if (file_descriptor == 0)
  {
    debug(VFSSYSCALL, "(close) Error: the fd does not exist.\n");
    return -1;
  }
  Inode* current_inode = file_descriptor->getFile()->getInode();
  assert(current_inode->getSuperblock()->removeFd(current_inode, file_descriptor) == 0);
  return 0;
}

int32 VfsSyscall::open(const char* pathname, uint32 flag)
{
  FileSystemInfo *fs_info = currentThread->getWorkingDirInfo();
  if (flag > (O_CREAT | O_RDWR))
  {
    debug(VFSSYSCALL, "(open) invalid parameter flag\n");
    return -1;
  }
  Dentry* pw_dentry = 0;
  VfsMount* pw_vfs_mount = 0;
  if (dupChecking(pathname, pw_dentry, pw_vfs_mount) == 0)
  {
    debug(VFSSYSCALL, "(open)current_dentry->getInode() \n");
    Inode* current_inode = pw_dentry->getInode();
    debug(VFSSYSCALL, "(open) current_inode->getSuperblock()\n");
    Superblock* current_sb = current_inode->getSuperblock();

    if (current_inode->getType() != I_FILE)
    {
      debug(VFSSYSCALL, "(open) Error: This path is not a file\n");
      return -1;
    }

    int32 fd = current_sb->createFd(current_inode, flag & 0xFFFFFFFB);
    debug(VFSSYSCALL, "the fd-num: %d, flag: %d\n", fd, flag);

    return fd;
  }
  else if (flag & O_CREAT)
  {
    debug(VFSSYSCALL, "(open) create a new file\n");
    size_t len = fs_info->pathname_.find_last_of("/");
    ustl::string sub_dentry_name = fs_info->pathname_.substr(len + 1, fs_info->pathname_.length() - len);
    // set directory
    fs_info->pathname_ = fs_info->pathname_.substr(0, len + 1);

    Dentry* pw_dentry = 0;
    VfsMount* pw_vfs_mount = 0;
    int32 success = PathWalker::pathWalk(fs_info->pathname_.c_str(), 0, pw_dentry, pw_vfs_mount);

    if (success != 0)
    {
      debug(VFSSYSCALL, "(open) path_walker failed\n\n");
      return -1;
    }

    Inode* current_inode = pw_dentry->getInode();
    Superblock* current_sb = current_inode->getSuperblock();

    if (current_inode->getType() != I_DIR)
    {
      debug(VFSSYSCALL, "(open) Error: This path is not a directory\n\n");
      return -1;
    }

    // create a new dentry
    Dentry *sub_dentry = new Dentry(pw_dentry);
    sub_dentry->d_name_ = sub_dentry_name;
    sub_dentry->setParent(pw_dentry);
    debug(VFSSYSCALL, "(open) calling create Inode\n");
    Inode* sub_inode = current_sb->createInode(sub_dentry, I_FILE);
    if (!sub_inode)
    {
      delete sub_dentry;
      return -1;
    }
    debug(VFSSYSCALL, "(open) created Inode with dentry name %s\n", sub_inode->getDentry()->getName());

    int32 fd = current_sb->createFd(sub_inode, flag & 0xFFFFFFFB);
    debug(VFSSYSCALL, "the fd-num: %d\n", fd);

    return fd;
  }
  else
    return -1;
}

int32 VfsSyscall::read(uint32 fd, char* buffer, uint32 count)
{
  FileDescriptor* file_descriptor = getFileDescriptor(fd);

  if (file_descriptor == 0)
  {
    debug(VFSSYSCALL, "(read) Error: the fd does not exist.\n");
    return -1;
  }

  if (count == 0)
    return 0;
  return file_descriptor->getFile()->read(buffer, count, 0);
}

int32 VfsSyscall::write(uint32 fd, const char *buffer, uint32 count)
{
  FileDescriptor* file_descriptor = getFileDescriptor(fd);

  if (file_descriptor == 0)
  {
    debug(VFSSYSCALL, "(write) Error: the fd does not exist.\n");
    return -1;
  }

  if (count == 0)
    return 0;
  return file_descriptor->getFile()->write(buffer, count, 0);
}

l_off_t VfsSyscall::lseek(uint32 fd, l_off_t offset, uint8 origin)
{
  FileDescriptor* file_descriptor = getFileDescriptor(fd);

  if (file_descriptor == 0)
  {
    debug(VFSSYSCALL, "(lseek) Error: the fd does not exist.\n");
    return -1;
  }

  return file_descriptor->getFile()->lseek(offset, origin);
}

int32 VfsSyscall::flush(uint32 fd)
{
  FileDescriptor* file_descriptor = getFileDescriptor(fd);

  if (file_descriptor == 0)
  {
    debug(VFSSYSCALL, "(read) Error: the fd does not exist.\n");
    return -1;
  }

  return file_descriptor->getFile()->flush();
}

int32 VfsSyscall::mount(const char *device_name, const char *dir_name, const char *file_system_name, int32 flag)
{
  FileSystemType* type = vfs.getFsType(file_system_name);
  if (!type && strcmp(file_system_name, "minixfs") == 0)
  {
    assert(vfs.registerFileSystem(new MinixFSType()) == 0);
  }
  else if (!type)
    return -1; // file system type not known

  return vfs.mount(device_name, dir_name, file_system_name, flag);
}

int32 VfsSyscall::umount(const char *dir_name, int32 flag)
{
  return vfs.umount(dir_name, flag);
}

uint32 VfsSyscall::getFileSize(uint32 fd)
{
  FileDescriptor* file_descriptor = getFileDescriptor(fd);

  if (file_descriptor == 0)
  {
    debug(VFSSYSCALL, "(read) Error: the fd does not exist.\n");
    return -1;
  }

  return file_descriptor->getFile()->getSize();
}
