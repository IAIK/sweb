/**
 * Filename: FsWorkingDirectory.cpp
 * Description:
 *
 * Created on: 09.07.2012
 * Author: chris
 */

#include "fs/FsWorkingDirectory.h"

#include "fs/VfsSyscall.h"
#include "fs/inodes/Inode.h"
#include "fs/inodes/Directory.h"

#ifdef USE_FILE_SYSTEM_ON_GUEST_OS
#include <cstring>
#include <string>
#include "debug_print.h"
#else
#include "ustl/ustring.h"
#include "kprintf.h"
#endif

FsWorkingDirectory::FsWorkingDirectory() : working_dir_path_(NULL), working_dir_(NULL),
    root_dir_path_(NULL), root_dir_(NULL)
{
  // initalise root-directory to the VFS-root
  changeRootDir(NULL);
  setWorkingDir(NULL);
}

FsWorkingDirectory::FsWorkingDirectory(const FsWorkingDirectory& cpy) :
    working_dir_path_(NULL), working_dir_(NULL),
    root_dir_path_(NULL), root_dir_(NULL)
{
  if(cpy.working_dir_path_ != NULL)
  {
    working_dir_path_ = strdup(cpy.working_dir_path_);
  }
  else
  {
    setWorkingDir(NULL);
  }

  if(cpy.root_dir_path_ == NULL && cpy.root_dir_ == NULL)
  {
    changeRootDir(NULL);
  }
  else
  {
    root_dir_path_ = strdup(cpy.root_dir_path_);
  }
}

FsWorkingDirectory::~FsWorkingDirectory()
{
  debug(FILE_SYSTEM, "~FsWorkingDirectory - CALL\n");

  deleteRootString();
  deleteWorkingString();
}

int32 FsWorkingDirectory::setWorkingDir(const char* working_dir)
{
  debug(FILE_SYSTEM, "setWorkingDir - CALL\n");

  // no new wd specified, just reset to root-directory
  if(working_dir == NULL)
  {
    // free if exists
    deleteWorkingString();

    working_dir_path_ = new char[2];
    working_dir_path_[0] = VfsSyscall::getLastDefinedPathSeparator();
    working_dir_path_[1] = '\0';
  }
  // copy given wd
  else
  {
    char path_separtor = VfsSyscall::getLastDefinedPathSeparator();
    uint32 new_wd_len = strlen(working_dir);

    // new path is an absolute path
    if(new_wd_len >= 1 && working_dir[0] == path_separtor)
    {
      // set given wd as new wd
      deleteWorkingString();
      working_dir_path_ = strdup(working_dir);
    }
    // new wd is relative to the current
    else
    {
      if(working_dir_path_ == NULL)
      {
        // set path to root
        setWorkingDir(NULL);
      }
      else
      {
        debug(FILE_SYSTEM, "setWorkingDir - cat new path to old\n");

        // cat new path to old
        char* old_path = strdup(working_dir_path_);
        uint32 old_path_len = strlen(old_path);

        debug(FILE_SYSTEM, "setWorkingDir - old \"%s\" new \"%s\"\n", old_path, working_dir);

        deleteWorkingString();

        if(old_path[old_path_len-1] == path_separtor)
        {
          // there is already a separator, so not an additional is required
          uint32 cat_wd_len = old_path_len + new_wd_len + 1;
          working_dir_path_ = new char[cat_wd_len];

          strncpy(working_dir_path_, old_path, old_path_len);
          strncpy(working_dir_path_ + old_path_len, working_dir, new_wd_len);
          working_dir_path_[cat_wd_len-1] = '\0';
        }
        else
        {
          uint32 cat_wd_len = old_path_len + 1 + new_wd_len + 1;
          working_dir_path_ = new char[cat_wd_len];

          strncpy(working_dir_path_, old_path, old_path_len);
          working_dir_path_[old_path_len] = path_separtor;
          strncpy(working_dir_path_ + old_path_len + 1, working_dir, new_wd_len);
          working_dir_path_[cat_wd_len-1] = '\0';
        }

        delete[] old_path;

      }
    }
  }

  debug(FILE_SYSTEM, "setWorkingDir - DONE new wd \"%s\"\n", working_dir_path_);

  return 0;
}

/*
char* FsWorkingDirectory::getWorkingDirPath(void) const
{
  // returning a copy of the Working-Directory string
  return strdup(working_dir_path_);
}
*/

const char* FsWorkingDirectory::getWorkingDirPath(void) const
{
  return working_dir_path_;
}

/*
Directory* FsWorkingDirectory::getWorkingDir(void)
{
  return working_dir_;
}
*/

void FsWorkingDirectory::changeRootDir(const char* new_root_dir)
{
  char path_separator = VfsSyscall::getLastDefinedPathSeparator();

  // new custom root is the VFS-root
  if(new_root_dir == NULL
      || ((strlen(new_root_dir) == 1 && new_root_dir[0] == path_separator)))
  {
    deleteRootString();

    root_dir_path_ = new char[2];
    root_dir_path_[0] = path_separator;
    root_dir_path_[1] = '\0';

    //root_dir_ = VfsSyscall::instance()->getVfsRoot();
  }
  else
  {
    //Directory* new_root = VfsSyscall::instance()->resolveDirectory(NULL, new_root_dir);

    //if(new_root == NULL)
      //return;

    // delete old root-directory string
    deleteRootString();

    // set new Root-dir
    root_dir_path_ = strdup(new_root_dir);
    //root_dir_ = new_root;
  }
}

/*
char* FsWorkingDirectory::getRootDirPath(void) const
{
  return strdup(root_dir_path_);
}
*/

const char* FsWorkingDirectory::getRootDirPath(void) const
{
  return root_dir_path_;
}

/*
Directory* FsWorkingDirectory::getRootDir(void)
{
  return root_dir_;
}
*/

void FsWorkingDirectory::deleteRootString(void)
{
  if(root_dir_path_ != NULL)
  {
    delete[] root_dir_path_;
    root_dir_path_ = NULL;
  }

  if(root_dir_ != NULL)
  {
    // decrement reference count of I-Node
    FileSystem* fs = root_dir_->getFileSystem();
    fs->releaseInode(root_dir_);
  }
}

void FsWorkingDirectory::deleteWorkingString(void)
{
  debug(FILE_SYSTEM, "deleteWorkingString - CALL %x\n", &working_dir_path_);

  if(working_dir_path_ != NULL)
  {
    debug(FILE_SYSTEM, "deleteWorkingString - deleting %s\n", working_dir_path_);

    delete[] working_dir_path_;
    working_dir_path_ = NULL;
  }

  if(working_dir_ != NULL)
  {
    debug(FILE_SYSTEM, "deleteWorkingString - releasing Directory* working_dir_\n");

    // decrement reference count of I-Node
    FileSystem* fs = working_dir_->getFileSystem();
    fs->releaseInode(working_dir_);
  }
}
