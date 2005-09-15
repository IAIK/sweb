// FileSystemInfo.cpp

#include "fs/FileSystemInfo.h"
#include "fs/Dentry.h"
#include "fs/VfsMount.h"
#include "util/string.h"
#include "mm/kmalloc.h"


/// A global object for information about the current position in the FileSystem.
/// TODO this has to be integrated in the ussr spave process code.
/// Every process needs one of these.
FileSystemInfo fs_info;

//---------------------------------------------------------------------------
FileSystemInfo::FileSystemInfo()
{
}

//---------------------------------------------------------------------------
int32 FileSystemInfo::setName(const char* pathname, uint32 length)
{
  if(pathname == 0)
    return -1;


  uint32 path_len = strlen(pathname);
  assert(length < path_len);
  if(length == 0)
    path_len += 1;
  else
    path_len = length + 1;

  kprintfd("pathname = %s, copy the length %d\n", pathname, (path_len - 1));
  pathname_ = (char*)kmalloc(path_len * sizeof(char));
  strlcpy(pathname_, pathname, path_len);
  kprintfd("pathname_ = %s, pathname = %s\n", pathname_, pathname);
  return 0;
}

//---------------------------------------------------------------------------
void FileSystemInfo::putName()
{
  if(pathname_ != 0)
  {
    kfree(pathname_);
    pathname_ = 0;
  }
}

