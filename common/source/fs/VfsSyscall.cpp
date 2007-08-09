// Projectname: SWEB
// Simple operating system for educational purposes

#include "fs/VfsSyscall.h"
#include "util/string.h"
#include "assert.h"
#include "fs/fs_global.h"
#include "mm/kmalloc.h"
#include "fs/Dirent.h"

#include "fs/Inode.h"
#include "fs/Dentry.h"
#include "fs/Superblock.h"
#include "fs/File.h"
#include "fs/FileDescriptor.h"
#include "fs/FileSystemType.h"

#include "fs/VfsMount.h"

#include "console/kprintf.h"

VfsSyscall vfs_syscall;

#define SEPARATOR '/'
#define CHAR_DOT '.'

//---------------------------------------------------------------------------
FileDescriptor* VfsSyscall::getFileDescriptor(uint32 fd)
{
  FileDescriptor* file_descriptor = 0;
  uint32 num = global_fd.getLength();
  for(uint32 counter = 0; counter < num; counter++)
  {
    if(global_fd.at(counter)->getFd() == fd)
    {
      file_descriptor = global_fd.at(counter);
      kprintfd("found the fd\n");
      break;
    }
  }
  return file_descriptor;
}

//---------------------------------------------------------------------------
int32 VfsSyscall::dupChecking(const char* pathname)
{
  FileSystemInfo *fs_info = currentThread->getFSInfo();
  if(pathname == 0)
    return -1;

  if((pathname[0] != SEPARATOR) && (pathname[1] != SEPARATOR) && 
     (pathname[2] != SEPARATOR))
  {
    uint32 path_len = strlen(pathname) + 1;
    char *path_tmp = (char*)kmalloc((path_len + 2) * sizeof(char));
                     // path_tmp = "./" + pathname + '\0'
    char *path_tmp_ptr = path_tmp;
    *path_tmp_ptr++ = CHAR_DOT;
    *path_tmp_ptr++ = SEPARATOR;
    strlcpy(path_tmp_ptr, pathname, path_len);
    
    fs_info->setName(path_tmp);
    kfree(path_tmp);
  } 
  else
    fs_info->setName(pathname);

  int32 success = path_walker.pathInit(fs_info->getName(), 0);
  if(success == 0)
    success = path_walker.pathWalk(fs_info->getName());

  // checked
  return success;
}

//---------------------------------------------------------------------------
int32 VfsSyscall::mkdir(const char* pathname, int32)
{
  kprintfd("(mkdir) \n");
  FileSystemInfo *fs_info = currentThread->getFSInfo();
  if(dupChecking(pathname) == 0)
  {
    kprintfd("(mkdir) the pathname exists\n");
    path_walker.pathRelease();
    fs_info->putName();
    return -1;
  }
  kprintfd("(mkdir) pathRelease();\n");
  path_walker.pathRelease();
  char* path_tmp=(char*)kmalloc((strlen(fs_info->getName())+ 1) * sizeof(char));
  strlcpy(path_tmp, fs_info->getName(), (strlen(fs_info->getName()) + 1));
  fs_info->putName();
  
  char* char_tmp = strrchr(path_tmp, SEPARATOR); 
  assert(char_tmp != 0);
  kprintfd("(mkdir)setName \n");
  // set directory
  uint32 path_prev_len = char_tmp - path_tmp + 1;
  fs_info->setName(path_tmp, path_prev_len);
    
  char* path_prev_name = fs_info->getName();
  kprintfd("(mkdir) path_prev_name: %s\n",path_prev_name);
  int32 success = path_walker.pathInit(path_prev_name, 0);
  if(success == 0)
    success = path_walker.pathWalk(path_prev_name);
  fs_info->putName();

  if(success != 0)
  {
    kprintfd("path_walker failed\n\n");
    path_walker.pathRelease();
    return -1;
  }
  
  Dentry* current_dentry = path_walker.getDentry();
  path_walker.pathRelease();
  Inode* current_inode = current_dentry->getInode();
  Superblock* current_sb = current_inode->getSuperblock();

  if(current_inode->getType() != I_DIR)
  {
    kprintfd("This path is not a directory\n\n");
    return -1;
  }

  char_tmp++;
  uint32 path_next_len = strlen(path_tmp) - path_prev_len + 1;
  char* path_next_name = (char*)kmalloc(path_next_len * sizeof(char));
  strlcpy(path_next_name, char_tmp, path_next_len);

  // create a new dentry
  Dentry *sub_dentry = new Dentry(current_dentry);
  sub_dentry->setName(path_next_name);
  kfree(path_next_name);
  kprintfd("(mkdir) creating Inode: current_dentry->getName(): %s\n",current_dentry->getName());
  kprintfd("(mkdir) creating Inode: sub_dentry->getName(): %s\n",sub_dentry->getName());
  kprintfd("(mkdir) current_sb: %d\n",current_sb);
  kprintfd("(mkdir) current_sb->getFSType(): %d\n",current_sb->getFSType());
//   kprintfd("(mkdir) current_sb->getFSType()->getFSName(): %d\n",current_sb->getFSType()->getFSName());
  kprintfd("(mkdir) \n\n\n");
  current_sb->createInode(sub_dentry, I_DIR);
  kprintfd("(mkdir) sub_dentry->getInode(): %d\n",sub_dentry->getInode());
  
 
  return 0;
}

Dirent* VfsSyscall::readdir(const char* pathname)
{
  FileSystemInfo *fs_info = currentThread->getFSInfo();
  if(dupChecking(pathname) == 0)
  {
    path_walker.pathRelease();
    char* path_tmp=(char*)kmalloc((strlen(fs_info->getName())+ 1) * sizeof(char));
    strlcpy(path_tmp, fs_info->getName(), (strlen(fs_info->getName()) + 1));
    fs_info->putName();
  
    char* char_tmp = strrchr(path_tmp, SEPARATOR);
    assert(char_tmp != 0);

    // set directory
    uint32 path_prev_len = char_tmp - path_tmp + 1;
    fs_info->setName(path_tmp, path_prev_len-1);
    
    char* path_prev_name = fs_info->getName();

    int32 success = path_walker.pathInit(path_prev_name, 0);
    if(success == 0)
    {
      success = path_walker.pathWalk(path_tmp);
    }
    fs_info->putName();

    if(success != 0)
    {
      kprintfd("(list) path_walker failed\n\n");
      path_walker.pathRelease();
      return((Dirent*)0);
    }

    Dentry* dentry = path_walker.getDentry();
    
    if(dentry->getInode()->getType() != I_DIR)
    {
      kprintfd("This path is not a directory\n\n");
      return (Dirent*)0;
    }
    
    kprintf("listing dir %s:\n",dentry->getName());
    uint32 num_child = dentry->getNumChild();
    for (uint32 i = 0; i<num_child; i++)
    {
      Dentry *sub_dentry = dentry->getChild(i);
      Inode* sub_inode = sub_dentry->getInode();
      uint32 inode_type = sub_inode->getType();
      switch(inode_type)
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
      kprintf("%s\n",sub_dentry->getName());
    }
  }
  else
  {
    kprintfd("(list) Path doesn't exist\n");
    path_walker.pathRelease();
  }
  return (Dirent*)0;
}

//---------------------------------------------------------------------------
int32 VfsSyscall::chdir(const char* pathname)
{
  FileSystemInfo *fs_info = currentThread->getFSInfo();
  if(dupChecking(pathname) != 0)
  {
    kprintfd("Error: (chdir) the directory does not exist.\n");
    path_walker.pathRelease();
    fs_info->putName();
    return -1;
  }

  fs_info->putName();
  Dentry* current_dentry = path_walker.getDentry();
  Inode* current_inode = current_dentry->getInode();
  if(current_inode->getType() != I_DIR)
  {
    kprintfd("This path is not a directory\n\n");
    path_walker.pathRelease();
    return -1;
  }

  fs_info->setFsPwd(path_walker.getDentry(), path_walker.getVfsMount());
  path_walker.pathRelease();

  return 0;
}

//---------------------------------------------------------------------------
int32 VfsSyscall::rm(const char* pathname)
{
  kprintfd("(rm) name: %s\n",pathname);
  FileSystemInfo *fs_info = currentThread->getFSInfo();
  if(dupChecking(pathname) != 0)
  {
    kprintfd("Error: (rm) the directory does not exist.\n");
    path_walker.pathRelease();
    fs_info->putName();
    return -1;
  }
  kprintfd("(rm) \n");
  fs_info->putName();
  Dentry* current_dentry = path_walker.getDentry();
  kprintfd("(rm) current_dentry->getName(): %s \n",current_dentry->getName());
  path_walker.pathRelease();
  Inode* current_inode = current_dentry->getInode();
  kprintfd("(rm) current_inode: %d\n",current_inode );

  if(current_inode->getType() != I_FILE)
  {
    kprintfd("This is not a file\n");
    return -1;
  }
  
  Superblock* sb = current_inode->getSuperblock();
  if(current_inode->rm() == INODE_DEAD)
  {
    kprintfd("remove the inode %d from the list of sb: %d\n",current_inode ,sb);
    sb->delete_inode(current_inode);
    kprintfd("removed\n");
  }
  else
  {
    kprintfd("remove the inode failed\n");
    return -1;
  }
  
  return 0;
}

//---------------------------------------------------------------------------
int32 VfsSyscall::rmdir(const char* pathname)
{
  FileSystemInfo *fs_info = currentThread->getFSInfo();
  if(dupChecking(pathname) != 0)
  {
    kprintfd("Error: (rmdir) the directory does not exist.\n");
    path_walker.pathRelease();
    fs_info->putName();
    return -1;
  }

  fs_info->putName();
  Dentry* current_dentry = path_walker.getDentry();
  path_walker.pathRelease();
  Inode* current_inode = current_dentry->getInode();

  if(current_dentry->getNumChild() != 0)
  {
    kprintfd("This directory is not empty\n");
    return -1;
  }
  
  if(current_inode->getType() != I_DIR)
  {
    kprintfd("This is not a directory\n");
    return -1;
  }

  Superblock* sb = current_inode->getSuperblock();
  if(current_inode->rmdir() == INODE_DEAD)
  {
    kprintfd("remove the inode from the list\n");
    sb->delete_inode(current_inode);
  }
  else
  {
    kprintfd("remove the inode failed\n");
    return -1;
  }
  
  return 0;
}

//---------------------------------------------------------------------------
int32 VfsSyscall::close(uint32 fd)
{
  File* file = 0;
  FileDescriptor* file_descriptor = 0;

  file_descriptor = getFileDescriptor(fd);
  
  if(file_descriptor == 0)
  {
    kprintfd("(close) Error: the fd does not exist.\n");
    return -1;
  }
  
  file = file_descriptor->getFile();
  Inode* current_inode = file->getInode();
  Superblock *current_sb = current_inode->getSuperblock();
  int32 tmp = current_sb->removeFd(current_inode, file_descriptor);
  assert(tmp == 0);

  return 0;
}

//---------------------------------------------------------------------------
int32 VfsSyscall::open(const char* pathname, uint32 flag)
{
  FileSystemInfo *fs_info = currentThread->getFSInfo();
  if(flag > O_RDWR)
  {
    kprintfd("(open) invalid parameter flag\n");
    return -1;
  }
  
  if(dupChecking(pathname) == 0)
  {
    kprintfd("(open) putName\n");
    fs_info->putName();
    kprintfd("(open) path_walker.getDentry()\n");
    Dentry* current_dentry = path_walker.getDentry();
    kprintfd("(open) pathRelease\n");
    path_walker.pathRelease();
    kprintfd("(open)current_dentry->getInode() \n");
    Inode* current_inode = current_dentry->getInode();
    kprintfd("(open) current_inode->getSuperblock()\n");
    Superblock* current_sb = current_inode->getSuperblock();
    kprintfd("(open)getNumOpenedFile() \n");
    uint32 num = current_inode->getNumOpenedFile();
    if(num > 0)
    { 
      kprintfd("(open) repeated open\n");
      // check the existing file
      if(flag != O_RDONLY)
      {
        kprintfd("(open) Error: The flag is not READ_ONLY\n");
        return -1;
      }
    
      if(current_inode->getType() != I_FILE)
      {
        kprintfd("(open) Error: This path is not a file\n\n");
        return -1;
      }

      File* file = current_inode->getFirstFile();
      uint32 file_flag = file->getFlag();
      if(file_flag != O_RDONLY)
      {
        kprintfd("(open) Error: The file flag is not READ_ONLY\n");
        return -1;
      }
    }
    
    int32 fd = current_sb->createFd(current_inode, flag);
    kprintfd("the fd-num: %d\n", fd);
  
    return fd;
  }
  else
  {
    kprintfd("(open) create a new file\n");
    path_walker.pathRelease();
    char* path_tmp=(char*)kmalloc((strlen(fs_info->getName())+ 1) * sizeof(char));
    strlcpy(path_tmp, fs_info->getName(), (strlen(fs_info->getName()) + 1));
    fs_info->putName();
  
    char* char_tmp = strrchr(path_tmp, SEPARATOR); 
    assert(char_tmp != 0)

    // set directory
    uint32 path_prev_len = char_tmp - path_tmp + 1;
    fs_info->setName(path_tmp, path_prev_len);
    
    char* path_prev_name = fs_info->getName();

    int32 success = path_walker.pathInit(path_prev_name, 0);
    if(success == 0)
      success = path_walker.pathWalk(path_prev_name);
    fs_info->putName();

    if(success != 0)
    {
      kprintfd("(open) path_walker failed\n\n");
      path_walker.pathRelease();
      return -1;
    }

    Dentry* current_dentry = path_walker.getDentry();
    path_walker.pathRelease();
    Inode* current_inode = current_dentry->getInode();
    Superblock* current_sb = current_inode->getSuperblock();

    if(current_inode->getType() != I_DIR)
    {
      kprintfd("(open) Error: This path is not a directory\n\n");
      return -1;
    }

    char_tmp++;
    uint32 path_next_len = strlen(path_tmp) - path_prev_len + 1;
    char* path_next_name = (char*)kmalloc(path_next_len * sizeof(char));
    strlcpy(path_next_name, char_tmp, path_next_len);

    // create a new dentry
    Dentry *sub_dentry = new Dentry( current_dentry );
    sub_dentry->setName(path_next_name);
    kfree(path_next_name);
    sub_dentry->setParent(current_dentry);
    kprintfd("(open) calling create Inode\n");
    Inode* sub_inode = current_sb->createInode(sub_dentry, I_FILE);
    kprintfd("(open) created Inode with dentry name %s\n", sub_inode->getDentry()->getName());
    
    if( !sub_inode )
    {
      delete sub_dentry;
      return -1;
    }
      
    int32 fd = current_sb->createFd(sub_inode, flag);
    kprintfd("the fd-num: %d\n", fd);
  
    return fd;
  }
}

//---------------------------------------------------------------------------
int32 VfsSyscall::read(uint32 fd, char* buffer, uint32 count)
{
  FileDescriptor* file_descriptor = 0;

  file_descriptor = getFileDescriptor(fd);

  if(file_descriptor == 0)
  {
    kprintfd("(read) Error: the fd does not exist.\n");
    return -1;
  }

  File* file = file_descriptor->getFile();
  return(file->read(buffer, count, 0));
}

//---------------------------------------------------------------------------
int32 VfsSyscall::write(uint32 fd, const char *buffer, uint32 count)
{
  FileDescriptor* file_descriptor = 0;
  
  file_descriptor = getFileDescriptor(fd);
  
  if(file_descriptor == 0)
  {
    kprintfd("(read) Error: the fd does not exist.\n");
    return -1;
  }
  
  File* file = file_descriptor->getFile();
  return(file->write(buffer, count, 0));
}

int32 VfsSyscall::flush(uint32 fd)
{
  FileDescriptor* file_descriptor = 0;
  
  file_descriptor = getFileDescriptor(fd);
  
  if(file_descriptor == 0)
  {
    kprintfd("(read) Error: the fd does not exist.\n");
    return -1;
  }
  
  File* file = file_descriptor->getFile();
  
  return file->flush();
}



