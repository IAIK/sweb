
#include "fs/ramfs/RamFsInode.h"

#include "mm/kmalloc.h"
#include "util/string.h"

#define BASIC_ALLOC 4096

RamFsInode::RamFsInode()
{
  data_ = (int32*)kmalloc(BASIC_ALLOC);
}

RamFsInode::~RamFsInode()
{
  if (data_)
  {
    kfree(data_);
  }
}

int32 RamFsInode::readData(int32 offset, int32 size, int32 *buffer)
{
  int32 *ptr_offset = data_ + (offset / 4);

  if (memcpy(buffer, ptr_offset, size))
  {
    return 1;
  }

  return 0;
}


