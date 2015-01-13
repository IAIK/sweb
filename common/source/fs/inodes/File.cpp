/**
 * @file File.cpp
 */

#include "fs/inodes/File.h"

#ifdef USE_FILE_SYSTEM_ON_GUEST_OS
#include "debug_print.h"
#else
#include "kprintf.h"
#endif

File::File(uint32 inode_number, uint32 device_sector, uint32 sector_offset,
        FileSystem* file_system, unix_time_stamp access_time,
        unix_time_stamp mod_time, unix_time_stamp c_time,
        uint32 ref_count, uint32 size)
    : Inode(inode_number, device_sector, sector_offset, file_system,
            access_time, mod_time, c_time, ref_count, size)
{
}

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
