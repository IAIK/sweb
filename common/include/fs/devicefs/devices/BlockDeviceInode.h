#pragma once

#include "Inode.h"

class BDVirtualDevice;

class BlockDeviceInode : public Inode
{
public:
    BlockDeviceInode(BDVirtualDevice* device);
    ~BlockDeviceInode() override = default;

    File* open(Dentry* dentry, uint32 /*flag*/) override;

    int32 readData(uint32 /*offset*/, uint32 /*size*/, char */*buffer*/) override;
    int32 writeData(uint32 /*offset*/, uint32 /*size*/, const char*/*buffer*/) override;
private:

    BDVirtualDevice* device_;
};
