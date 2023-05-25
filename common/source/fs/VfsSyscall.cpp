#include "VfsSyscall.h"

#include "Dentry.h"
#include "Dirent.h"
#include "File.h"
#include "FileDescriptor.h"
#include "FileSystemInfo.h"
#include "FileSystemType.h"
#include "Inode.h"
#include "MinixFSType.h"
#include "PathWalker.h"
#include "Superblock.h"
#include "VfsMount.h"
#include "VirtualFileSystem.h"
#include "kprintf.h"
#include "kstring.h"

#include "assert.h"

#ifndef EXE2MINIXFS
#include "Thread.h"
#endif

#define SEPARATOR '/'
#define CHAR_DOT '.'

FileDescriptor* VfsSyscall::getFileDescriptor(uint32 fd)
{
  return FileDescriptorList::globalFdList().getFileDescriptor(fd);
}


int32 VfsSyscall::mkdir(const char* pathname, int32)
{
  debug(VFSSYSCALL, "(mkdir) Path: %s\n", pathname);

  FileSystemInfo *fs_info = getcwd();

  Path target_path;
  Path parent_dir_path;
  int32 path_walk_status = PathWalker::pathWalk(pathname, fs_info, target_path, &parent_dir_path);

  if((path_walk_status == PW_ENOTFOUND) && parent_dir_path.dentry_)
  {
    eastl::string new_dir_name = PathWalker::lastPathSegment(pathname, true);

    Inode* parent_dir_inode = parent_dir_path.dentry_->getInode();
    Superblock* parent_dir_sb = parent_dir_inode->getSuperblock();

    if (parent_dir_inode->getType() != I_DIR)
    {
      debug(VFSSYSCALL, "(mkdir) Error: This path is not a directory\n");
      return -1;
    }

    debug(VFSSYSCALL, "(mkdir) Creating new directory Inode in superblock %p of type %s\n", parent_dir_sb, parent_dir_sb->getFSType()->getFSName());
    Inode* new_dir_inode = parent_dir_sb->createInode(I_DIR);

    debug(VFSSYSCALL, "(mkdir) Creating new dentry: %s in parent dir %s, inode: %p\n", new_dir_name.c_str(), parent_dir_path.dentry_->getName(), new_dir_inode);
    Dentry* new_dir_dentry = new Dentry(new_dir_inode, parent_dir_path.dentry_, new_dir_name);
    new_dir_inode->mkdir(new_dir_dentry);

    return 0;
  }
  else if (path_walk_status == PW_SUCCESS)
  {
    debug(VFSSYSCALL, "(mkdir) Error: The pathname already exists\n");
  }
  else if (path_walk_status == PW_EINVALID)
  {
    debug(VFSSYSCALL, "(mkdir) Error: Invalid pathname\n");
  }
  else if ((path_walk_status == PW_ENOTFOUND) && !parent_dir_path.dentry_)
  {
    debug(VFSSYSCALL, "(mkdir) Error: Parent directory not found\n\n");
  }

  return -1;
}


ssize_t VfsSyscall::getdents(int fd, char* buffer, size_t buffer_size)
{
    debug(VFSSYSCALL, "(getdents) Getting dentries for fd: %d\n", fd);

    if (!buffer)
    {
        debug(VFSSYSCALL, "(getdents) Error: buffer = NULL\n");
        return -1;
    }

    FileDescriptor* file_descriptor = getFileDescriptor(fd);
    if (file_descriptor == nullptr)
    {
        debug(VFSSYSCALL, "(getdents) Error: The fd does not exist.\n");
        return -1;
    }

    if (file_descriptor->getFile()->getInode()->getType() != I_DIR)
    {
        debug(VFSSYSCALL, "(getdents) Error: fd is not a directory.\n");
        return -1;
    }

    Dentry* dir_dentry = file_descriptor->getFile()->getDentry();
    debug(VFSSYSCALL, "(getdents) Reading dentries for dir %p (inode %p) %s\n",
          dir_dentry, dir_dentry->getInode(), dir_dentry->getName());

    ssize_t buf_offs = 0;
    user_dirent* u_dirent = nullptr;
    user_dirent* u_dirent_prev = nullptr;
    for (Dentry* sub_dentry : dir_dentry->d_child_)
    {
        uint32 d_type = sub_dentry->getInode()->getType();
        const char* d_name = sub_dentry->getName();
        auto d_name_len = strlen(d_name) + 1;

        debug(VFSSYSCALL, "(getdents) child %s, type: %x\n", d_name, d_type);

        u_dirent = (user_dirent*)(buffer + buf_offs);

        size_t u_dirent_size = sizeof(*u_dirent) + d_name_len;

        if (buf_offs + u_dirent_size > buffer_size)
        {
            debug(VFSSYSCALL, "(getdents) Error: Buffer is too small for all dirents. Size: %zu\n", buffer_size);
            return -1;
        }

        u_dirent->d_offs_next = 0;
        u_dirent->d_type = d_type;
        memcpy(u_dirent->d_name, d_name, d_name_len);

        buf_offs += u_dirent_size;

        if (u_dirent_prev)
        {
            u_dirent_prev->d_offs_next = (char*)u_dirent - (char*)u_dirent_prev;
        }
        u_dirent_prev = u_dirent;
    }

    return buf_offs;
}

int32 VfsSyscall::chdir(const char* pathname)
{
  debug(VFSSYSCALL, "(chdir) Changing working directory to: %s\n", pathname);

  FileSystemInfo *fs_info = getcwd();

  Path target_path;
  if (PathWalker::pathWalk(pathname, fs_info, target_path) != PW_SUCCESS)
  {
    debug(VFSSYSCALL, "(chdir) Error: The directory does not exist.\n");
    return -1;
  }

  Inode* current_inode = target_path.dentry_->getInode();
  if (current_inode->getType() != I_DIR)
  {
    debug(VFSSYSCALL, "(chdir) Error: This path is not a directory\n");
    return -1;
  }

  fs_info->setPwd(target_path);

  return 0;
}

int32 VfsSyscall::rm(const char* pathname)
{
  debug(VFSSYSCALL, "(rm) Removing: %s\n", pathname);

  FileSystemInfo *fs_info = getcwd();

  Path target_path;
  if (PathWalker::pathWalk(pathname, fs_info, target_path) != PW_SUCCESS)
  {
    debug(VFSSYSCALL, "(rm) target file does not exist.\n");
    return -1;
  }
  debug(VFSSYSCALL, "(rm) current_dentry->getName(): %s \n", target_path.dentry_->getName());
  Inode* current_inode = target_path.dentry_->getInode();
  debug(VFSSYSCALL, "(rm) current_inode: %p\n", current_inode);

  if (current_inode->getType() != I_FILE)
  {
    debug(VFSSYSCALL, "(rm) Target is not a file\n");
    return -1;
  }

  if (current_inode->unlink(target_path.dentry_))
  {
    debug(VFSSYSCALL, "(rm) Error: File unlink failed\n");
    return -1;
  }

  delete target_path.dentry_;
  if (!current_inode->numRefs())
  {
    Superblock* sb = current_inode->getSuperblock();
    debug(VFSSYSCALL, "(rm) No more references to inode left, remove inode %p from the list of sb: %p\n", current_inode, sb);
    sb->deleteInode(current_inode);
  }

  return 0;
}

int32 VfsSyscall::rmdir(const char* pathname)
{
  FileSystemInfo *fs_info = getcwd();

  Path target_dir;
  Path parent_dir_path;
  int32 path_walk_status = PathWalker::pathWalk(pathname, fs_info, target_dir, &parent_dir_path);

  if(path_walk_status != PW_SUCCESS)
  {
    debug(VFSSYSCALL, "(rmdir) Error: The directory does not exist.\n");
    return -1;
  }

  Inode* current_inode = target_dir.dentry_->getInode();

  //if directory is read from a real file system,
  //it can't ever be empty, "." and ".." are still
  //contained; this has to be checked in Inode::rmdir()

  if (current_inode->getType() != I_DIR)
  {
    debug(VFSSYSCALL, "(rmdir) Error: This is not a directory\n");
    return -1;
  }

  if (current_inode->rmdir(target_dir.dentry_))
  {
    debug(VFSSYSCALL, "(rmdir) Error: File system rmdir failed\n");
    return -1;
  }

  delete target_dir.dentry_;
  if (!current_inode->numRefs())
  {
    Superblock* sb = current_inode->getSuperblock();
    debug(VFSSYSCALL, "(rmdir) No more references to inode left, remove inode %p from the list of sb: %p\n", current_inode, sb);
    sb->deleteInode(current_inode);
  }

  return 0;
}

int32 VfsSyscall::close(uint32 fd)
{
  debug(VFSSYSCALL, "(close) Close fd num %u\n", fd);

  FileDescriptor* file_descriptor = getFileDescriptor(fd);

  if (file_descriptor == nullptr)
  {
    debug(VFSSYSCALL, "(close) Error: the fd does not exist.\n");
    return -1;
  }

  assert(!FileDescriptorList::globalFdList().remove(file_descriptor));
  file_descriptor->getFile()->closeFd(file_descriptor);

  debug(VFSSYSCALL, "(close) File closed\n");
  return 0;
}

int32 VfsSyscall::open(const char* pathname, uint32 flag)
{
  if(!pathname)
  {
    debug(VFSSYSCALL, "(open) Invalid pathname\n");
    return -1;
  }

#ifndef EXE2MINIXFS
  // MutexLock l(vfs_lock); // TODO: When userspace pagefault occurs while holding vfs lock -> pagefault handler attempts to load page data -> calls vfs read -> lock vfs lock -> deadlock
#endif


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
  int32 path_walk_status = PathWalker::pathWalk(pathname, fs_info, target_path, &parent_dir_path);

  if (path_walk_status == PW_SUCCESS)
  {
    debug(VFSSYSCALL, "(open) Found target file: %s\n", target_path.dentry_->getName());
    Inode* target_inode = target_path.dentry_->getInode();
    assert(target_inode);

    File* file = target_inode->open(target_path.dentry_, flag);
    if (!file)
    {
        debug(VFSSYSCALL, "(open) Unable to open file\n");
        return -1;
    }
    FileDescriptor* fd = file->openFd();
    assert(!FileDescriptorList::globalFdList().add(fd));

    debug(VFSSYSCALL, "(open) Fd for new open file: %d, flags: %x\n", fd->getFd(), flag);
    return fd->getFd();
  }
  else if ((path_walk_status == PW_ENOTFOUND) && (flag & O_CREAT))
  {
    if (!parent_dir_path.dentry_)
    {
      debug(VFSSYSCALL, "(open) ERROR: unable to create file, directory does not exist\n");
      return -1;
    }

    debug(VFSSYSCALL, "(open) target does not exist, creating new file\n");

    eastl::string new_dentry_name = PathWalker::lastPathSegment(pathname);
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

    debug(VFSSYSCALL, "(open) Creating new file Inode in superblock %p of type %s\n", parent_dir_sb, parent_dir_sb->getFSType()->getFSName());
    Inode* new_file_inode = parent_dir_sb->createInode(I_FILE);
    if (!new_file_inode)
    {
      debug(VFSSYSCALL, "(open) ERROR: Unable to create inode for new file\n");
      return -1;
    }

    debug(VFSSYSCALL, "(open) Creating new dentry: %s in parent dir %s\n", new_dentry_name.c_str(), parent_dir_path.dentry_->getName());
    Dentry* new_file_dentry = new Dentry(new_file_inode, parent_dir_path.dentry_, new_dentry_name);
    new_file_inode->mkfile(new_file_dentry);

    File* file = new_file_inode->open(new_file_dentry, flag);
    FileDescriptor* fd = file->openFd();
    assert(!FileDescriptorList::globalFdList().add(fd));

    debug(VFSSYSCALL, "(open) Fd for new open file: %d, flags: %x\n", fd->getFd(), flag);
    return fd->getFd();
  }
  else if (path_walk_status == PW_EINVALID)
  {
    debug(VFSSYSCALL, "(open) Error: Invalid pathname\n");
  }
  else
  {
    debug(VFSSYSCALL, "(open) Error: File not found\n");
  }
  return -1;
}

int32 VfsSyscall::read(uint32 fd, char* buffer, uint32 count)
{
  FileDescriptor* file_descriptor = getFileDescriptor(fd);

  if (file_descriptor == nullptr)
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
  debug(VFSSYSCALL, "(write) Write %u bytes from %p to fd %u\n", count, buffer, fd);

  FileDescriptor* file_descriptor = getFileDescriptor(fd);

  if (file_descriptor == nullptr)
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

  if (file_descriptor == nullptr)
  {
    debug(VFSSYSCALL, "(lseek) Error: the fd does not exist.\n");
    return -1;
  }

  return file_descriptor->getFile()->lseek(offset, origin);
}

int32 VfsSyscall::flush(uint32 fd)
{
  FileDescriptor* file_descriptor = getFileDescriptor(fd);

  if (file_descriptor == nullptr)
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
      return -1;
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

  if (file_descriptor == nullptr)
  {
    debug(VFSSYSCALL, "(read) Error: the fd does not exist.\n");
    return -1;
  }

  return file_descriptor->getFile()->getSize();
}
