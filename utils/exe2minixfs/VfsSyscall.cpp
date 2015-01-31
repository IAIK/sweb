#include <iostream>
#include "VfsSyscall.h"
#include <assert.h>
#include "Inode.h"
#include "Dentry.h"
#include "Superblock.h"
#include "MinixFSSuperblock.h"
#include "File.h"
#include "FileDescriptor.h"
#include "FileSystemInfo.h"
#include "PathWalker.h"
#include <cstring>

extern FileSystemInfo *fs_info;
extern std::list<FileDescriptor*> global_fd;
extern Superblock* superblock_;
extern PathWalker path_walker_;

#define SEPARATOR '/'
#define CHAR_DOT '.'

#define O_RDONLY    0x0000
#define O_WRONLY    0x0001
#define O_RDWR      0x0002
#define O_CREAT     0x0004

FileDescriptor* VfsSyscall::getFileDescriptor(uint32 fd)
{
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

int32 VfsSyscall::close(uint32 fd)
{
  FileDescriptor* file_descriptor = getFileDescriptor(fd);

  if (file_descriptor == 0)
  {
    debug(VFSSYSCALL, "(close) Error: the fd does not exist.\n");
    return -1;
  }

  Inode* current_inode = file_descriptor->getFile()->getInode();
  Superblock *current_sb = current_inode->getSuperblock();
  assert(current_sb->removeFd(current_inode, file_descriptor) == 0);

  return 0;
}

int32 VfsSyscall::open(const char* pathname, uint32 flag)
{
  debug(VFSSYSCALL, "(open)\n");
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
    debug(VFSSYSCALL, "(open)getNumOpenedFile() \n");

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
    char path_tmp[strlen(fs_info->pathname_.c_str()) + 1];
    strncpy(path_tmp, fs_info->pathname_.c_str(), (strlen(fs_info->pathname_.c_str()) + 1));
    path_tmp[strlen(fs_info->pathname_.c_str())] = 0;

    char* char_tmp = strrchr(path_tmp, SEPARATOR);
    assert(char_tmp != 0);

    // set directory
    uint32 path_prev_len = char_tmp - path_tmp + 1;
    fs_info->pathname_ = path_tmp;
    fs_info->pathname_ = fs_info->pathname_.substr(0, path_prev_len);

    Dentry* pw_dentry = 0;
    VfsMount* pw_vfs_mount = 0;
    int32 success = PathWalker::pathWalk(fs_info->pathname_.c_str(), 0, pw_dentry, pw_vfs_mount);

    if (success != 0)
    {
      debug(VFSSYSCALL, "(open) path_walker failed\n\n");
      return -1;
    }

    Dentry* current_dentry = pw_dentry;
    Inode* current_inode = current_dentry->getInode();
    Superblock* current_sb = current_inode->getSuperblock();

    if (current_inode->getType() != I_DIR)
    {
      debug(VFSSYSCALL, "(open) Error: This path is not a directory\n\n");
      return -1;
    }

    char_tmp++;
    uint32 path_next_len = strlen(path_tmp) - path_prev_len + 1;
    char path_next_name[path_next_len];
    strncpy(path_next_name, char_tmp, path_next_len);
    path_next_name[path_next_len - 1] = 0;

    // create a new dentry
    Dentry *sub_dentry = new Dentry(current_dentry);
    sub_dentry->setName(path_next_name);
    sub_dentry->setParent(current_dentry);
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

int32 VfsSyscall::write(uint32 fd, const char *buffer, uint32 count)
{
  FileDescriptor* file_descriptor = getFileDescriptor(fd);

  if (file_descriptor == 0)
  {
    debug(VFSSYSCALL, "(write) Error: the fd does not exist.\n");
    return -1;
  }

  return file_descriptor->getFile()->write(buffer, count, 0);
}
