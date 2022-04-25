#pragma once

#include "Inode.h"

class BDVirtualDevice;

class BlockDeviceInode : public Inode
{
public:
    BlockDeviceInode(BDVirtualDevice* device);
    virtual ~BlockDeviceInode() = default;

    virtual File* open(Dentry* dentry, uint32 /*flag*/);

    virtual int32 readData(uint32 /*offset*/, uint32 /*size*/, char */*buffer*/);
    virtual int32 writeData(uint32 /*offset*/, uint32 /*size*/, const char*/*buffer*/);
private:

    BDVirtualDevice* device_;
};
