#include "fs/VfsSyscall.h"
#include "util/string.h"
#include "assert.h"
#include "fs/fs_global.h"
#include "mm/kmalloc.h"
#include "fs/Dirent.h"

#include "fs/Inode.h"
#include "fs/Dentry.h"
#include "fs/Superblock.h"
#include "console/kprintf.h"

VfsSyscall vfs_syscall;

#define SEPARATOR '/'
#define CHAR_DOT '.'

//---------------------------------------------------------------------------
int32 VfsSyscall::dupChecking(const char* pathname)
{
  if(pathname == 0)
    return -1;

  char *path_tmp_ptr = 0;
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
    kprintfd("########### special and path_tmp = %s\n", path_tmp);
    
    fs_info.setName(path_tmp);
    kfree(path_tmp);
  } 
  else
    fs_info.setName(pathname);

  char* test_name = fs_info.getName();
  kprintfd("test_name_len = %s, has length %d\n", test_name, strlen(test_name));

  // fs_info.getName() = directory

  int32 success = path_walker.pathInit(fs_info.getName(), 0);
  kprintfd("after pathInit() success = %d\n", success);
  if(success == 0)
    success = path_walker.pathWalk(fs_info.getName());
  kprintfd("after pathWalk() success = %d\n", success);

  return success;
}

//---------------------------------------------------------------------------
int32 VfsSyscall::mkdir(const char* pathname, int32)
{
  kprintfd("***** start of syscall mkdir()\n");

  if(dupChecking(pathname) == 0)
  {
    kprintfd("the pathname is used\n");
    path_walker.pathRelease();
    fs_info.putName();
    return -1;
  }
  
  path_walker.pathRelease();
  char* path_tmp=(char*)kmalloc((strlen(fs_info.getName())+ 1) * sizeof(char));
  strlcpy(path_tmp, fs_info.getName(), (strlen(fs_info.getName()) + 1));
  fs_info.putName();
  
  char* char_tmp = strrchr(path_tmp, SEPARATOR); 
  assert(char_tmp != 0)

  kprintfd("set directory\n");
  uint32 path_prev_len = char_tmp - path_tmp + 1;
  fs_info.setName(path_tmp, path_prev_len);
    
  char* path_prev_name = fs_info.getName();
  kprintfd("path_prev_name = %s\n", path_prev_name);

  int32 success = path_walker.pathInit(path_prev_name, 0);
  kprintfd("after pathInit() success = %d\n", success);
  if(success == 0)
    success = path_walker.pathWalk(path_prev_name);
  fs_info.putName();
  kprintfd("after pathWalk() success = %d\n", success);

  if(success != 0)
  {
    kprintfd("path_walker failed\n\n");
    path_walker.pathRelease();
    return -1;
  }
  
  kprintfd("get the dentry of the path_walker\n");
  Dentry* current_dentry = path_walker.getDentry();
  path_walker.pathRelease();
  Inode* current_inode = current_dentry->getInode();
  Superblock* current_sb = current_inode->getSuperblock();

  if(current_inode->getMode() != I_DIR)
  {
    kprintfd("This path is not a directory\n\n");
    return -1;
  }

  kprintfd("create the path_next_name\n");
  char_tmp++;
  uint32 path_next_len = strlen(path_tmp) - path_prev_len + 1;
  kprintfd("path_next_len = %d\n", path_next_len);
  char* path_next_name = (char*)kmalloc(path_next_len * sizeof(char));
  strlcpy(path_next_name, char_tmp, path_next_len);
  kprintfd("path_next_name = %s\n", path_next_name);

  kprintfd("create a new dentry\n");
  Dentry *sub_dentry = new Dentry(current_dentry);
  sub_dentry->setName(path_next_name);
  kfree(path_next_name);
  kprintfd("number of the child: %d\n", current_dentry->getNumChild());
  current_sb->createInode(sub_dentry, I_DIR);
 
  kprintfd("***** end of syscall mkdir()\n");
  return 0;
}

//---------------------------------------------------------------------------
Dirent* VfsSyscall::readdir(const char* pathname)
{
  kprintfd("***** start of syscall readdir()\n");

  if(dupChecking(pathname) != 0)
  {
    kprintfd("Error: (readdir) the directory does not exist.\n");
    path_walker.pathRelease();
    fs_info.putName();
    return (Dirent*)0;
  }

  fs_info.putName();
  kprintfd("get the dentry of the path_walker\n");
  Dentry* current_dentry = path_walker.getDentry();
  path_walker.pathRelease();
  Inode* current_inode = current_dentry->getInode();

  if(current_inode->getMode() != I_DIR)
  {
    kprintfd("This path is not a directory\n\n");
    return (Dirent*)0;
  }
  
  for(uint32 counter = 0; counter < current_dentry->getNumChild(); counter++)
  {
    Dentry* sub_dentry = current_dentry->getChild(counter);
    Inode* sub_inode = sub_dentry->getInode();
    uint32 inode_mode = sub_inode->getMode();
    
    switch(inode_mode)
    {
      case I_DIR:
        kprintfd("[D]");
        break;
      case I_FILE:
        kprintfd("[F]");
        break;
      case I_LNK:
        kprintfd("[L]");
        break;
      default:
        break;
    }
    kprintfd("%s\n", sub_dentry->getName());
  }

  kprintfd("***** end of syscall readdir()\n");
  return((Dirent*)0);
}

//---------------------------------------------------------------------------
int32 VfsSyscall::chdir(const char* pathname)
{
  kprintfd("***** start of syscall chdir()\n");

  if(dupChecking(pathname) != 0)
  {
    kprintfd("Error: (chdir) the directory does not exist.\n");
    path_walker.pathRelease();
    fs_info.putName();
    return -1;
  }

  fs_info.putName();
  kprintfd("get the dentry of the path_walker\n");
  Dentry* current_dentry = path_walker.getDentry();
  Inode* current_inode = current_dentry->getInode();

  if(current_inode->getMode() != I_DIR)
  {
    kprintfd("This path is not a directory\n\n");
    path_walker.pathRelease();
    return -1;
  }

  fs_info.setFsPwd(path_walker.getDentry(), path_walker.getVfsMount());
  path_walker.pathRelease();

  kprintfd("***** end of syscall chddir()\n");
  return 0;
}

//---------------------------------------------------------------------------
int32 VfsSyscall::rmdir(const char* pathname)
{
  kprintfd("***** start of syscall rmdir()\n");

  if(dupChecking(pathname) != 0)
  {
    kprintfd("Error: (rmdir) the directory does not exist.\n");
    path_walker.pathRelease();
    fs_info.putName();
    return -1;
  }

  fs_info.putName();
  kprintfd("get the dentry of the path_walker\n");
  Dentry* current_dentry = path_walker.getDentry();
  path_walker.pathRelease();
  Inode* current_inode = current_dentry->getInode();

  if(current_inode->getMode() != I_DIR)
  {
    kprintfd("This file is not a directory\n");
    return -1;
  }

  Superblock* sb = current_inode->getSuperblock();
  kprintfd("~~~~ rmdir()\n");
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
  
  kprintfd("***** end of syscall rmdir()\n");
  return 0;
}

/*
int32 VfsSyscall::rmdir(char* dir)
{
  kprintfd("***** start of syscall rmdir()\n");
  if(dir == 0)
    return -1;

  fs_info.setName(dir);
  
  char* test_name = fs_info.getName();
  kprintfd("test_name_len = %s, has length %d\n", test_name, strlen(test_name));

  // fs_info.getName() = directory

  int32 success = path_walker.pathInit(fs_info.getName(), 0);
  kprintfd("after pathInit() success = %d\n", success);
  if(success == 0)
    success = path_walker.pathWalk(fs_info.getName());
  kprintfd("after pathWalk() success = %d\n", success);

  kprintfd("get the dentry of the path_walker\n");
  Dentry* current_dentry = path_walker.getDentry();
  Inode* current_inode = current_dentry->getInode();
  if(current_inode->getMode() != I_DIR)
  {
    kprintfd("This file is not a directory\n");
    path_walker.pathRelease();
    fs_info.putName();
    return -1;
  }

  Superblock* sb = current_inode->getSuperblock();
  kprintfd("~~~~ rmdir()\n");
  if(current_inode->rmdir() == INODE_DEAD)
  {
    kprintfd("remove the inode from the list\n");
    sb->delete_inode(current_inode);
  }
  else
  {
    kprintfd("remove the inode failed\n");
    path_walker.pathRelease();
    fs_info.putName();
    return -1;
  }
  
  path_walker.pathRelease();
  fs_info.putName();
 
  kprintfd("***** end of syscall rmdir()\n");
  
  return 0;
}
*/
