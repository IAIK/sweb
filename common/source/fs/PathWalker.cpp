
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
  char * cur_part = 0;

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

  cur_part = getNextPart(pos);
  if (!cur_part)
  {
    return -1;
  }

  // TODO get inode for every part
  Inode *current_inode = dentry_->get_inode();

  return 0;
}

//----------------------------------------------------------------------
char *PathWalker::getNextPart(const char* path)
{
  const char* tmp = 0;
  char *npart = 0;

  if ((tmp = strchr(path, '/')))
  {
    int32 len = strlcpy(npart,path, (size_t)(tmp -path));

    if (len >= (tmp -path))
    {
      return (char *)0;
    }

  }
  else
  {
  }
}
