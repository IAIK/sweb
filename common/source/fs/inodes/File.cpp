/**
 * @file File.cpp
 */

#include "fs/inodes/File.h"
//#include "fs/inodes/Inode.h"

File::File(uint32 inode_number, uint32 device_sector, uint32 sector_offset,
        FileSystem* file_system, unix_time_stamp access_time,
        unix_time_stamp mod_time, unix_time_stamp c_time,
        uint32 ref_count, uint32 size)
    : Inode(inode_number, device_sector, sector_offset, file_system,
            access_time, mod_time, c_time, ref_count, size)//, size_(size)
{
}

/*
File::File ( Inode* inode, Dentry* dentry, uint32 flag )
{
  f_inode_ = inode;
  f_dentry_ = dentry;
  flag_ = flag;
}
*/

File::~File()
{
  debug(FS_INODE, "~File - DONE\n");
}

Inode::InodeType File::getType(void) const
{
  return InodeTypeFile;
}

bool File::truncateProtected(void)
{
  getLock()->acquireWriteBlocking();
  bool result = truncateUnprotected();
  getLock()->releaseWrite();

  return result;
}
