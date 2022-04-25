#pragma once

#include "fs/File.h"

// No special operations by default
class RamFSFile : public SimpleFile
{
  public:

    RamFSFile(Inode* inode, Dentry* dentry, uint32 flag);

    virtual ~RamFSFile() = default;
};
