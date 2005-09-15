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
    
    fs_info.setName(path_tmp);
    kfree(path_tmp);
  } 
  else
    fs_info.setName(pathname);

  char* test_name = fs_info.getName();

  int32 success = path_walker.pathInit(fs_info.getName(), 0);
  if(success == 0)
    success = path_walker.pathWalk(fs_info.getName());

  return success;
}

//---------------------------------------------------------------------------
int32 VfsSyscall::mkdir(const char* pathname, int32)
{
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

  // set directory
  uint32 path_prev_len = char_tmp - path_tmp + 1;
  fs_info.setName(path_tmp, path_prev_len);
    
  char* path_prev_name = fs_info.getName();

  int32 success = path_walker.pathInit(path_prev_name, 0);
  if(success == 0)
    success = path_walker.pathWalk(path_prev_name);
  fs_info.putName();

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

  if(current_inode->getMode() != I_DIR)
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
  current_sb->createInode(sub_dentry, I_DIR);
 
  return 0;
}

//---------------------------------------------------------------------------
Dirent* VfsSyscall::readdir(const char* pathname)
{
  if(dupChecking(pathname) != 0)
  {
    kprintfd("Error: (readdir) the directory does not exist.\n");
    path_walker.pathRelease();
    fs_info.putName();
    return (Dirent*)0;
  }

  fs_info.putName();
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

  return((Dirent*)0);
}

//---------------------------------------------------------------------------
int32 VfsSyscall::chdir(const char* pathname)
{
  if(dupChecking(pathname) != 0)
  {
    kprintfd("Error: (chdir) the directory does not exist.\n");
    path_walker.pathRelease();
    fs_info.putName();
    return -1;
  }

  fs_info.putName();
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

  return 0;
}

//---------------------------------------------------------------------------
int32 VfsSyscall::rmdir(const char* pathname)
{

  if(dupChecking(pathname) != 0)
  {
    kprintfd("Error: (rmdir) the directory does not exist.\n");
    path_walker.pathRelease();
    fs_info.putName();
    return -1;
  }

  fs_info.putName();
  Dentry* current_dentry = path_walker.getDentry();
  path_walker.pathRelease();
  Inode* current_inode = current_dentry->getInode();

  if(current_inode->getMode() != I_DIR)
  {
    kprintfd("This file is not a directory\n");
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
