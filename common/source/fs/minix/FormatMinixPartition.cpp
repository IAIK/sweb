/**
 * Filename: FormatMinixPartition.cpp
 * Description:
 *
 * Created on: 06.09.2012
 * Author: chris
 */

#include "fs/minix/FormatMinixPartition.h"

#include "fs/VfsSyscall.h"

#ifdef USE_FILE_SYSTEM_ON_GUEST_OS
#include <cstring>
#include <math.h>
#include <assert.h>
#include "debug_print.h"
#else
#include "util/string.h"
#include "util/math.h"
#include "kprintf.h"
#endif

FormatMinixPartition::FormatMinixPartition()
{
}

FormatMinixPartition::~FormatMinixPartition()
{
}

bool FormatMinixPartition::formatSetupSuperblock(minix_super_block* superblock, FsDevice* device,
                                            sector_addr_t zone_size, int32 minix_version,
                                            int32 filename_len)
{
  sector_addr_t num_blocks = device->getNumBlocks();
  if(num_blocks > 0xFFFF)
  {
    debug(FS_MINIX, "format - NOTE device has %d block which is more that minix can address!\n");
    num_blocks = 0xFFFF;
  }

  // assume that the average sweb - file has about 10k, which means that
  // it uses 10 block of data, so there are 1/10*num_blocks inodes required
  uint16 num_inodes = num_blocks / 10 + 1;

  // calculate all the offsets

  // the InodeTable is one data-block behind the Superblock
  sector_addr_t inode_bitmap_size_ = (num_inodes) / (MINIX_BLOCK_SIZE*8);
  if((num_inodes) % (MINIX_BLOCK_SIZE*8) > 0)
  {
    inode_bitmap_size_++;
  }

  uint16 inode_size = MINIX_INODE_SIZE;
  if(minix_version == 2) inode_size = MINIX_V2_INODE_SIZE;

  sector_addr_t inode_tbl_size_ = (num_inodes * inode_size) / MINIX_BLOCK_SIZE;

  if((num_inodes * inode_size) % MINIX_BLOCK_SIZE > 0)
  {
    inode_tbl_size_++;
  }

  // calculate how many zones we have today
  uint16 num_zones = num_blocks - (2 + inode_bitmap_size_ + inode_tbl_size_);
  uint16 zone_bitmap_len = num_zones / (MINIX_BLOCK_SIZE*8);
  if(zone_bitmap_len % (MINIX_BLOCK_SIZE*8) != 0)
  {
    zone_bitmap_len++;
  }

  num_zones -= zone_bitmap_len;

  sector_addr_t zone_bitmap_size_ = num_zones / (MINIX_BLOCK_SIZE*8);
  if((num_zones) % (MINIX_BLOCK_SIZE*8) > 0)
  {
    zone_bitmap_size_++;
  }

  // check if the all calculations are correct, by computing the address of the first
  // data zone and comparing the value to the one stored in the superblock
  sector_addr_t first_data_zone = (MINIX_SUPERBLOCK_OFFSET + MINIX_BLOCK_SIZE) / MINIX_BLOCK_SIZE +
                                  inode_bitmap_size_ + zone_bitmap_size_ +
                                  inode_tbl_size_;

  uint32 max_filesize = 7+512+(512*512) * MINIX_BLOCK_SIZE;
  if(minix_version == 2)
  {
    max_filesize += pow(512, 3);
  }

  if(max_filesize > num_zones * MINIX_BLOCK_SIZE)
  {
    max_filesize = num_zones * MINIX_BLOCK_SIZE;
  }

  // determining the magic number (by default select Version 1 with 30 char filenames)
  uint16 magic_no = 0x138F;

  if(minix_version == 1 && filename_len == 14) magic_no = 0x137F;
  else if(minix_version == 2)
  {
    if(filename_len == 14) magic_no = 0x2468;
    else if(filename_len == 30) magic_no = 0x2478;
  }

  // determine log-zone_size
  uint16 log_zone_size = zone_size;
  if(log_zone_size > 10) log_zone_size = 0;

  // write new Superblock - determine used sizes
  superblock->s_ninodes       = num_inodes;
  superblock->s_nzones        = num_zones;
  superblock->s_imap_blocks   = inode_bitmap_size_;
  superblock->s_zmap_blocks   = zone_bitmap_size_;
  superblock->s_firstdatazone = first_data_zone;
  superblock->s_log_zone_size = log_zone_size;
  superblock->s_max_size      = max_filesize;
  superblock->s_magic         = magic_no;
  superblock->s_state         = MINIX_MOUNT_FLAG_OK;
  superblock->s_zones         = superblock->s_nzones;

  // print a short summary
  debug(FS_MINIX, "format - Summary: new Minix V%d\n", minix_version);
  debug(FS_MINIX, "has %d blocks (sized %d byte)\n", num_blocks, MINIX_BLOCK_SIZE);
  debug(FS_MINIX, "has %d zones (sized 1024<<%d byte)\n", num_zones, log_zone_size);
  debug(FS_MINIX, "has %d inodes\n", num_inodes);

  debug(FS_MINIX, "inode_bitmap_size %d\n", inode_bitmap_size_);
  debug(FS_MINIX, "zone_bitmap_size %d\n", zone_bitmap_size_);
  debug(FS_MINIX, "inode_tbl_size %d\n", inode_tbl_size_);
  debug(FS_MINIX, "first-zone %x\n", first_data_zone);

  return true;
}

bool FormatMinixPartition::cleanUpDataBlocks(FsDevice* device, sector_addr_t first_sector, sector_addr_t last_sector)
{
  // a clean data-block with all values set to 0x00
  char* clean_block = new char[MINIX_BLOCK_SIZE];
  memset(clean_block, 0x00, MINIX_BLOCK_SIZE);

  // clean all blocks including the first data-zone
  for(uint16 i = first_sector; i <= last_sector; i++)
  {
    if(!device->writeSector(i, clean_block, MINIX_BLOCK_SIZE))
    {
      debug(FS_MINIX, "format - ERROR failed to clear sector %x\n", i);

      delete[] clean_block;
      return false;
    }
  }

  delete[] clean_block;
  return true;
}

bool FormatMinixPartition::setRootInodeInBitmaps(FsDevice* device, minix_super_block* sb)
{
  char* bitmap_buf = new char[MINIX_BLOCK_SIZE];
  memset(bitmap_buf, 0x00, MINIX_BLOCK_SIZE);

  // set the first bit:
  bitmap_buf[0] = 0x01;

  sector_addr_t inode_bitmap_sector = (MINIX_SUPERBLOCK_OFFSET / MINIX_BLOCK_SIZE) + 1;
  sector_addr_t zone_bitmap_sector = inode_bitmap_sector + sb->s_imap_blocks;

  debug(FS_MINIX, "setRootInodeInBitmaps - Inode-Bitmap-block=%x Zone-Bitmap-Block=%x\n", inode_bitmap_sector, zone_bitmap_sector);

  // write to disk
  bool ret1 = device->writeSector(inode_bitmap_sector, bitmap_buf, MINIX_BLOCK_SIZE);
  bool ret2 = device->writeSector(zone_bitmap_sector, bitmap_buf, MINIX_BLOCK_SIZE);

  delete[] bitmap_buf;

  if(!ret1 || !ret2)
    return false;

  return true;
}

bool FormatMinixPartition::setRootInodeInTable(FsDevice* device,
    minix_super_block* sb, int32 minix_version, int32 dir_entry_size)
{
  char* inode_tbl_buffer = new char[MINIX_BLOCK_SIZE];
  memset(inode_tbl_buffer, 0x00, MINIX_BLOCK_SIZE);

  // create the root-inode and set it to the InodeTable
  if(minix_version == 1)
  {
    minix_inode* root_inode = reinterpret_cast<minix_inode*>(inode_tbl_buffer);

    root_inode->i_mode = S_IFDIR | 0755;
    root_inode->i_uid = 0;
    root_inode->i_size = 2*dir_entry_size;
    root_inode->i_time = VfsSyscall::getCurrentTimeStamp();
    root_inode->i_gid = 0;
    root_inode->i_nlinks = 2;

    for(uint16 i = 0; i < 9; i++)
      root_inode->i_zone[i] = 0x00;

    root_inode->i_zone[0] = sb->s_firstdatazone;
  }
  else if(minix_version == 2)
  {
    minix2_inode* root_inode = reinterpret_cast<minix2_inode*>(inode_tbl_buffer);

    root_inode->i_mode = S_IFDIR | 0755;
    root_inode->i_nlinks = 2;
    root_inode->i_uid = 0;
    root_inode->i_gid = 0;
    root_inode->i_size = 2*dir_entry_size;
    root_inode->i_atime = VfsSyscall::getCurrentTimeStamp();
    root_inode->i_mtime = VfsSyscall::getCurrentTimeStamp();
    root_inode->i_ctime = VfsSyscall::getCurrentTimeStamp();

    for(uint16 i = 0; i < 10; i++)
      root_inode->i_zone[i] = 0x00;

    root_inode->i_zone[0] = sb->s_firstdatazone;
  }

  sector_addr_t inode_tbl_sector = (MINIX_SUPERBLOCK_OFFSET / MINIX_BLOCK_SIZE) + 1 + sb->s_imap_blocks + sb->s_zmap_blocks;

  debug(FS_MINIX, "setRootInodeInTable - inode-table sector=%x\n", inode_tbl_sector);

  // write the root-inode to the device
  bool ret = device->writeSector(inode_tbl_sector, inode_tbl_buffer, MINIX_BLOCK_SIZE);

  delete[] inode_tbl_buffer;
  return ret;
}

bool FormatMinixPartition::setRootInodeInZone(FsDevice* device, minix_super_block* sb, int32 filename_len)
{
  // create "." and ".." for the root-inode
  char* root_inode_data = new char[MINIX_BLOCK_SIZE];
  memset(root_inode_data, 0x00, MINIX_BLOCK_SIZE);

  // create "."
  root_inode_data[0x00] = 0x1;
  root_inode_data[0x02] = '.';

  // create ".."
  root_inode_data[0x00 + filename_len + 2] = 0x1;
  root_inode_data[0x02 + filename_len + 2] = '.';
  root_inode_data[0x03 + filename_len + 2] = '.';

  // write to disk
  bool ret = device->writeSector(sb->s_firstdatazone, root_inode_data, MINIX_BLOCK_SIZE);
  debug(FS_MINIX, "setRootInodeInZone - first-zone=%x\n", sb->s_firstdatazone);

  delete[] root_inode_data;
  return ret;
}

bool FormatMinixPartition::createRootInode(FsDevice* device, minix_super_block* sb, int32 minix_version, int32 filename_len)
{
  // 1. occupy the first bit in the InodeTable and in the ZoneBitmap
  if(!setRootInodeInBitmaps(device, sb))
  {
    return false;
  }

  // 2. create Inode in InodeTable
  int32 dir_entry_size = filename_len + 2;
  if(!setRootInodeInTable(device, sb, minix_version, dir_entry_size))
  {
    return false;
  }

  // 3. setup data-zone of root-inode
  if(!setRootInodeInZone(device, sb, filename_len))
  {
    return false;
  }

  return true;
}

bool FormatMinixPartition::formatWriteSuperblock(FsDevice* device, minix_super_block* sb)
{
  char* sb_buffer = new char[MINIX_BLOCK_SIZE];
  memset(sb_buffer, 0x00, MINIX_BLOCK_SIZE);

  // copy SB struct ptr into char array
  memcpy(sb_buffer, sb, sizeof(minix_super_block));

  // writing to Device
  bool ret = device->writeSector(1, sb_buffer, MINIX_BLOCK_SIZE);

  delete[] sb_buffer;
  return ret;
}

bool FormatMinixPartition::format(FsDevice* device, sector_addr_t zone_size, int32 minix_version, int32 filename_len)
{
  assert(device != NULL);
  debug(FS_MINIX, "format - going to format given device with Minix V%d\n", minix_version);

  // set Minix block-size
  device->setBlockSize(MINIX_BLOCK_SIZE);

  // setup the super-block:
  minix_super_block sb;
  if(!formatSetupSuperblock(&sb, device, zone_size, minix_version, filename_len))
  {
    debug(FS_MINIX, "format - ERROR failed to setup the Superblock\n");
    return false;
  }

  // write super-block to disk
  if(!formatWriteSuperblock(device, &sb))
  {
    debug(FS_MINIX, "format - ERROR failed to write the Superblock\n");
    return false;
  }

  // cleaning all important data-blocks (setting all values there to 0x00)
  if(!cleanUpDataBlocks(device, 2, sb.s_firstdatazone))
  {
    debug(FS_MINIX, "format - ERROR failed to clean up fs\n");
    return false;
  }

  // finally create the root-inode
  if(!createRootInode(device, &sb, minix_version, filename_len))
  {
    debug(FS_MINIX, "format - ERROR failed to create root inode\n");
    return false;
  }

  debug(FS_MINIX, "format - DONE successfully!\n");
  return true;
}
