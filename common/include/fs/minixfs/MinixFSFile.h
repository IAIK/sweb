#pragma once

#include "File.h"

class MinixFSFile : public SimpleFile
{
public:
    /**
     * constructor
     * @param inode the inode of the file
     * @param dentry the dentry
     * @param flag the flag i.e. readonly
     */
    MinixFSFile(Inode* inode, Dentry* dentry, uint32 flag);

    ~MinixFSFile() override = default;

    /**
     * writes all data to disc
     * @return 0 on success
     */
    int32 flush() override;
};
