#include "VfsSyscall.h"
#include "kstring.h"
#include "assert.h"
#include "Dirent.h"
#include "Inode.h"
#include "Dentry.h"
#include "Superblock.h"
#include "File.h"
#include "FileDescriptor.h"
#include "FileSystemType.h"
#include "FileSystemInfo.h"
#include "VirtualFileSystem.h"
#include "MinixFSType.h"
#include "PathWalker.h"
#include "VfsMount.h"
#include "kprintf.h"
#ifndef EXE2MINIXFS
#include "Mutex.h"
#include "Thread.h"
#endif

#define SEPARATOR '/'
#define CHAR_DOT '.'

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

int32 VfsSyscall::dupChecking(const char* pathname, FileSystemInfo* fs_info, Path& out_path, Path* parent_dir)
{
    return dupChecking(pathname, fs_info->getPwd(), fs_info->getRoot(), out_path, parent_dir);
}

int32 VfsSyscall::dupChecking(const char* pathname, const Path& pwd, const Path& root, Path& out_path, Path* parent_dir)
{
  FileSystemInfo *fs_info = getcwd();
  assert(fs_info != NULL);
  assert(fs_info->pathname_.c_str() != NULL);
  if (pathname == 0)
    return -1;

  uint32 len = strlen(pathname);
  fs_info->pathname_ = "./";

  for (size_t i = 0; i < 3; ++i)
  {
    if (len > i && pathname[i] == SEPARATOR)
      fs_info->pathname_ = "";
    else if (pathname[i] != CHAR_DOT)
      break;
  }
  fs_info->pathname_ += pathname;

  return PathWalker::pathWalk(fs_info->pathname_.c_str(), pwd, root, 0, out_path, parent_dir);
}

int32 VfsSyscall::mkdir(const char* pathname, int32)
{
  debug(VFSSYSCALL, "(mkdir) Path: %s\n", pathname);
  FileSystemInfo *fs_info = getcwd();

  Path target_path;
  if (dupChecking(pathname, fs_info, target_path) == 0)
  {
    debug(VFSSYSCALL, "(mkdir) The pathname already exists\n");
    return -1;
  }

  // Find parent directory
  uint32 len = fs_info->pathname_.find_last_of("/");
  ustl::string sub_dentry_name = fs_info->pathname_.substr(len+1, fs_info->pathname_.length() - len);
  // set parent directory path
  fs_info->pathname_ = fs_info->pathname_.substr(0, len);

  Path parent_dir_path;
  int32 success = PathWalker::pathWalk(fs_info->pathname_.c_str(), fs_info, 0, parent_dir_path);

  if (success != 0)
  {
    debug(VFSSYSCALL, "(mkdir) Parent directory not found\n\n");
    return -1;
  }

  Inode* parent_dir_inode = parent_dir_path.dentry_->getInode();
  Superblock* parent_dir_sb = parent_dir_inode->getSuperblock();

  if (parent_dir_inode->getType() != I_DIR)
  {
    debug(VFSSYSCALL, "(mkdir) This path is not a directory\n");
    return -1;
  }

  debug(VFSSYSCALL, "(mkdir) Creating new directory Inode in superblock %p of type %s\n", parent_dir_sb, parent_dir_sb->getFSType()->getFSName());
  Inode* new_dir_inode = parent_dir_sb->createInode(I_DIR);

  debug(VFSSYSCALL, "(mkdir) Creating new direntry: %s in parent dir %s\n", sub_dentry_name.c_str(), parent_dir_path.dentry_->getName());
  Dentry* new_dir_dentry = new Dentry(new_dir_inode, parent_dir_path.dentry_, sub_dentry_name);
  new_dir_inode->mkdir(new_dir_dentry);
  return 0;
}

Dirent* VfsSyscall::readdir(const char* pathname)
{
  FileSystemInfo *fs_info = getcwd();
  Path target_dir;
  // TODO: dup checking is unnecessary here, just check if pathWalk succeeds or not
  if (dupChecking(pathname, fs_info, target_dir) == 0)
  {
    int32 success = PathWalker::pathWalk(fs_info->pathname_.c_str(), fs_info, 0, target_dir);

    if (success != 0)
    {
      debug(VFSSYSCALL, "(readdir) ERROR: Target directory not found\n");
      return nullptr;
    }

    if (target_dir.dentry_->getInode()->getType() != I_DIR)
    {
      debug(VFSSYSCALL, "(readdir) ERROR: This path is not a directory\n\n");
      return nullptr;
    }

    debug(VFSSYSCALL, "(readdir) listing dir %s:\n", target_dir.dentry_->getName());
    for (Dentry* sub_dentry : target_dir.dentry_->d_child_)
    {
      uint32 inode_type = sub_dentry->getInode()->getType();
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
  FileSystemInfo *fs_info = getcwd();
  Path target_dir;
  if (dupChecking(pathname, fs_info, target_dir) != 0)
  {
    debug(VFSSYSCALL, "Error: (chdir) the directory does not exist.\n");
    return -1;
  }

  Inode* current_inode = target_dir.dentry_->getInode();
  if (current_inode->getType() != I_DIR)
  {
    debug(VFSSYSCALL, "This path is not a directory\n\n");
    return -1;
  }

  fs_info->setFsPwd(target_dir);

  return 0;
}

int32 VfsSyscall::rm(const char* pathname)
{
  debug(VFSSYSCALL, "(rm) name: %s\n", pathname);

  FileSystemInfo *fs_info = getcwd();
  Path target_path;
  if (dupChecking(pathname, fs_info, target_path) != 0)
  {
    debug(VFSSYSCALL, "(rm) target file does not exist.\n");
    return -1;
  }
  debug(VFSSYSCALL, "(rm) current_dentry->getName(): %s \n", target_path.dentry_->getName());
  Inode* current_inode = target_path.dentry_->getInode();
  debug(VFSSYSCALL, "(rm) current_inode: %p\n", current_inode);

  if (current_inode->getType() != I_FILE)
  {
    debug(VFSSYSCALL, "This is not a file\n");
    return -1;
  }

  Superblock* sb = current_inode->getSuperblock();
  if (current_inode->rm() == INODE_DEAD)
  {
    debug(VFSSYSCALL, "remove the inode %p from the list of sb: %p\n", current_inode, sb);
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
  FileSystemInfo *fs_info = getcwd();
  Path target_dir;
  if (dupChecking(pathname, fs_info, target_dir) != 0)
  {
    debug(VFSSYSCALL, "Error: (rmdir) the directory does not exist.\n");
    return -1;
  }

  Inode* current_inode = target_dir.dentry_->getInode();

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
  if(!pathname || strlen(pathname) == 0)
  {
    debug(VFSSYSCALL, "(open) Invalid pathname\n");
    return -1;
  }

  debug(VFSSYSCALL, "(open) Opening file %s\n", pathname);
  if (flag & ~(O_RDONLY | O_WRONLY | O_CREAT | O_RDWR | O_TRUNC | O_APPEND))
  {
    debug(VFSSYSCALL, "(open) Invalid flag parameter\n");
    return -1;
  }
  if(flag & (O_APPEND | O_TRUNC))
  {
    kprintfd("(open) ERROR: Flags not yet implemented\n"); // kprintfd instead of debug so it will be printed even if the debug flag is disabled
    return -1;
  }

  FileSystemInfo *fs_info = getcwd();

  Path target_path;
  Path parent_dir_path;

  int32 path_walk_status = PathWalker::pathWalk(pathname, fs_info, 0, target_path, &parent_dir_path);
  if (path_walk_status == PW_SUCCESS)
  {
    debug(VFSSYSCALL, "(open) Found target file\n");
    Inode* target_inode = target_path.dentry_->getInode();
    Superblock* target_sb = target_inode->getSuperblock();

    if (target_inode->getType() != I_FILE)
    {
      debug(VFSSYSCALL, "(open) Error: This path is not a file\n");
      return -1;
    }

    int32 fd = target_sb->createFd(target_inode, flag);
    debug(VFSSYSCALL, "(open) Fd for new open file: %d, flags: %x\n", fd, flag);

    return fd;
  }
  else if ((path_walk_status == PW_ENOTFOUND) && (flag & O_CREAT))
  {
    if (!parent_dir_path.dentry_)
    {
      debug(VFSSYSCALL, "(open) ERROR: unable to create file, directory does not exist\n");
      return -1;
    }

    debug(VFSSYSCALL, "(open) target does not exist, creating new file\n");

    ustl::string new_dentry_name = PathWalker::lastPathSegment(pathname);
    if(new_dentry_name == "") // Path has trailing slashes
    {
      debug(VFSSYSCALL, "(open) Error: last path segment is empty (trailing '/')\n");
      return -1;
    }

    debug(VFSSYSCALL, "(open) new file name: %s\n", new_dentry_name.c_str());

    Inode* parent_dir_inode = parent_dir_path.dentry_->getInode();
    Superblock* parent_dir_sb = parent_dir_inode->getSuperblock();

    if (parent_dir_inode->getType() != I_DIR)
    {
      debug(VFSSYSCALL, "(open) ERROR: This path is not a directory\n");
      return -1;
    }

    debug(VFSSYSCALL, "(open) Creating inode for new file\n");
    Inode* new_file_inode = parent_dir_sb->createInode(I_FILE);
    if (!new_file_inode)
    {
      debug(VFSSYSCALL, "(open) ERROR: Unable to create inode for new file\n");
      return -1;
    }

    debug(VFSSYSCALL, "(open) Creating dentry for new file\n");
    Dentry* new_file_dentry = new Dentry(new_file_inode, parent_dir_path.dentry_, new_dentry_name);
    new_file_inode->mkfile(new_file_dentry);

    int32 fd = parent_dir_sb->createFd(new_file_inode, flag);
    debug(VFSSYSCALL, "(open) Fd for new open file: %d\n", fd);

    return fd;
  }
  else
  {
    debug(VFSSYSCALL, "(open) File not found\n");
    return -1;
  }
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

#ifndef EXE2MINIXFS
int32 VfsSyscall::mount(const char *device_name, const char *dir_name, const char *file_system_name, int32 flag)
{
  FileSystemType* type = vfs.getFsType(file_system_name);
  if (!type)
  {
      debug(VFSSYSCALL, "(mount) Unknown file system %s\n", file_system_name);
      return -1; // file system type not known
  }

  return vfs.mount(device_name, dir_name, file_system_name, flag);
}

int32 VfsSyscall::umount(const char *dir_name, int32 flag)
{
  return vfs.umount(dir_name, flag);
}
#endif
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
