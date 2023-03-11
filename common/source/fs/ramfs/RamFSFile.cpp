#include "fs/ramfs/RamFSFile.h"

#include "fs/Dentry.h"
#include "fs/ramfs/RamFSInode.h"
#include "fs/ramfs/RamFSSuperblock.h"

#define ERROR_FRO "ERROR: The flag muss be READONLY for several opened files"
#define ERROR_FF  "ERROR: The flag does not allow this operation"
#define ERROR_FNO "ERROR: The file is not open."

RamFSFile::RamFSFile(Inode* inode, Dentry* dentry, uint32 flag) :
    SimpleFile(inode, dentry, flag)
{
}
