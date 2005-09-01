
#include "fs/PathWalker.h"

#include "fs/Inode.h"

#include "assert.h"
#include "util/string.h"

//----------------------------------------------------------------------
PathWalker::PathWalker()
{
}

//----------------------------------------------------------------------
PathWalker::~PathWalker()
{
}

//----------------------------------------------------------------------
int32 PathWalker::init(const char* name, uint32 flags)
{
  assert(name);

  flags_ = flags;

  /// TODO get PWD and Mount from current process

  bool from_root = false;
  const char* pos = name;
  char cur_part[MAX_NAME_LEN];

  if (*pos++ == '/')
  {
    from_root = true;
  }
  while (*pos++ == '/');

  if (*(getNextPart(pos)) == '\0')
  {
    return 0;
  }

  // TODO adjust lookup flag according to process

  char *tmp = 0;
  if (tmp = strchr(pos, '/'))
  {
    size_t part_len = static_cast<size_t>(tmp - pos);
    if (part_len > MAX_NAME_LEN)
    {
      return -1;
    }
    int32 len = strlcpy(cur_part, pos, part_len);
    if (len >= part_len)
    {
      return -1;
    }
  }

  // TODO get inode for every part
  Inode *current_inode = dentry_->get_inode();
  char * cur_name = dentry_->get_name();

  return 0;
}


//----------------------------------------------------------------------
char *PathWalker::getNextPart(const char* path)
{
  const char* tmp = 0;
  char *npart = 0;

  if ((tmp = strchr(path, '/')))
  {
    int32 len = strlcpy(npart, path, (size_t)(tmp -path));

    if (len >= (tmp - path))
    {
      return (char *)0;
    }

  }
}

////----------------------------------------------------------------------
//char *PathWalker::skipSeparator(char const *path) const
//{
//  assert(path);
//
//  while (*path == '/')
//  {
//    ++path;
//  }
//
//  return path;
//}

