#include "PathWalker.h"
#include "Inode.h"
#include "Dentry.h"
#include "VfsMount.h"
#include "Superblock.h"
#include "assert.h"
#include "kstring.h"
#include "kprintf.h"
#include "FileSystemInfo.h"
#include "Path.h"
#ifndef EXE2MINIXFS
#include "Mutex.h"
#include "Thread.h"
#endif

#define CHAR_DOT '.'
#define NULL_CHAR '\0'
#define CHAR_ROOT '/'
#define SEPARATOR '/'

int32 PathWalker::pathWalk(const char* pathname, const Path& pwd, const Path& root, uint32 flags_ __attribute__ ((unused)), Path& out)
{
  // Flag indicating the type of the last path component.
  int32 last_type_ = 0;

  // The last path component
  char* last_ = 0;

  if (pathname == 0)
  {
    debug(PATHWALKER, "pathWalk> ERROR: pathname is null\n");
    return PW_ENOTFOUND;
  }

  // check the first character of the path
  if (*pathname == CHAR_ROOT)
  {
    // Start at ROOT
    last_type_ = LAST_ROOT;
    out = root;
  }
  else
  {
    // Start at PWD
    out = pwd;
  }

  if ((out.dentry_ == 0) || (out.mnt_ == 0))
  {
    debug(PATHWALKER, "PathWalker: PathWalk> ERROR return not found - dentry: %p, vfs_mount: %p\n", out.dentry_, out.mnt_);
    return PW_ENOTFOUND;
  }
  debug(PATHWALKER, "PathWalk> Start dentry: %p, vfs_mount: %p\n", out.dentry_, out.mnt_);

  debug(PATHWALKER, "pathWalk> pathname: %s\n", pathname);


  while (*pathname == SEPARATOR)
    pathname++;
  if (!*pathname) // i.e. path = /
  {
    debug(PATHWALKER, "pathWalk> return 0 pathname == \\n\n");
    return PW_SUCCESS;
  }

  bool parts_left = true;
  while (parts_left)
  {
    int32 npart_pos = 0;
    int32 npart_len = getNextPartLen(pathname, npart_pos);
    char npart[npart_len];
    strncpy(npart, pathname, npart_len);
    npart[npart_len - 1] = 0;
    debug(PATHWALKER, "pathWalk> npart : %s\n", npart);
    debug(PATHWALKER, "pathWalk> npart_pos : %d\n", npart_pos);
    if (npart_pos < 0)
    {
      debug(PATHWALKER, "pathWalk> return path invalid npart_pos < 0 \n");
      return PW_EINVALID;
    }

    if ((*npart == NULL_CHAR) || (npart_pos == 0))
    {
      debug(PATHWALKER, "pathWalk> return success\n");
      return PW_SUCCESS;
    }
    pathname += npart_pos;

    last_ = npart;
    if (*npart == CHAR_DOT)
    {
      if (*(npart + 1) == NULL_CHAR)
      {
        last_type_ = LAST_DOT;
      }
      else if ((*(npart + 1) == CHAR_DOT) && (*(npart + 2) == NULL_CHAR))
      {
        last_type_ = LAST_DOTDOT;
      }
    }
    else
    {
      last_type_ = LAST_NORM;
    }

    // follow the inode
    // check the VfsMount
    if (last_type_ == LAST_DOT) // follow LAST_DOT
    {
      debug(PATHWALKER, "pathWalk> follow last dot\n");
      last_ = 0;
      continue;
    }
    else if (last_type_ == LAST_DOTDOT) // follow LAST_DOTDOT
    {
      debug(PATHWALKER, "pathWalk> follow last dotdot\n");
      last_ = 0;

      if(out == root)
      {
        debug(PATHWALKER, "pathWalk> Reached global file system root\n");
        continue;
      }

#ifndef EXE2MINIXFS
      if(out.dentry_->getParent() == out.dentry_)
      {
        debug(PATHWALKER, "pathWalk> file system mount root reached, going up a mount to vfsmount %p, mountpoint %p %s\n", out.mnt_->getParent(), out.mnt_->getMountPoint(), out.mnt_->getMountPoint()->getName());

        out.dentry_ = out.mnt_->getMountPoint();
        out.mnt_ = out.mnt_->getParent();
      }
#endif
      out.dentry_ = out.dentry_->getParent();
      continue;
    }
    else if (last_type_ == LAST_NORM) // follow LAST_NORM
    {
      debug(PATHWALKER, "pathWalk> follow last norm last_: %s\n", last_);
      Inode* current_inode = out.dentry_->getInode();
      Dentry *found = current_inode->lookup(last_);
      if(!found)
      {
        debug(PATHWALKER, "pathWalk> dentry %s not found\n", last_);
        return PW_ENOTFOUND;
      }

      debug(PATHWALKER, "pathWalk> found dentry %s\n", found->getName());

      last_ = 0;
      out.dentry_ = found;

#ifndef EXE2MINIXFS
      VfsMount* vfs_mount = vfs.getVfsMount(out.dentry_);
      if (vfs_mount != 0)
      {
        debug(PATHWALKER, "Reached mountpoint at %s, going down to mounted file system\n", out.dentry_->getName());
        assert(out.dentry_->getMountedRoot() == vfs_mount->getRoot());

        out.mnt_ = vfs_mount;
        out.dentry_ = vfs_mount->getRoot();
      }
#endif
    }

    while (*pathname == SEPARATOR)
      pathname++;

    if (strlen(pathname) == 0)
    {
      break;
    }
  }
  debug(PATHWALKER, "pathWalk> return 0 end of function\n");

  return PW_SUCCESS;
}

int32 PathWalker::getNextPartLen(const char* path, int32 &npart_len)
{
  char* tmp = 0;
  tmp = strchr((char*) path, SEPARATOR);

  npart_len = (size_t) (tmp - path + 1);

  uint32 length = npart_len;

  if (tmp == 0)
  {
    npart_len = strlen(path);
    length = npart_len + 1;
  }

  return length;
}
