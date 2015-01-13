/**
 * Filename: InodeTableMinix.cpp
 * Description:
 *
 * Created on: 06.06.2012
 * Author: chris
 */

#include "fs/minix/InodeTableMinix.h"
#include "fs/minix/FileSystemMinix.h"
#include "fs/minix/MinixDataStructs.h"
#include "fs/minix/MinixDefs.h"
#include "fs/FsVolumeManager.h"

#include "fs/inodes/Inode.h"
#include "fs/inodes/Directory.h"
#include "fs/inodes/File.h"
#include "fs/inodes/RegularFile.h"

#ifdef USE_FILE_SYSTEM_ON_GUEST_OS
#include "debug_print.h"
#else
#include "kprintf.h"
#endif

InodeTableMinix::InodeTableMinix(FileSystemUnix* fs, FsVolumeManager* volume_manager,
    sector_addr_t inode_table_sector_start, sector_addr_t num_sectors,
    sector_addr_t inode_bitmap_start, sector_addr_t inode_bitmap_end,
    bitmap_t inode_bitmap_num_bits) :
    InodeTable(fs, volume_manager, inode_table_sector_start, num_sectors,
        inode_bitmap_start, inode_bitmap_end, inode_bitmap_num_bits)
{
}

InodeTableMinix::~InodeTableMinix()
{
}

Inode* InodeTableMinix::createInodeFromDeviceData(inode_id_t id,
    sector_addr_t sector, sector_len_t offset, minix_inode* node_data)
{
  Inode* inode = NULL;

  if(node_data->i_mode & S_IFDIR)
  {
    inode = new Directory(id, sector, offset, fs_, node_data->i_time, 0, 0, node_data->i_nlinks, node_data->i_size);
    debug(INODE_TABLE, "InodeTableMinix::createInodeFromDeviceData - Directory instance created\n");
  }
  else if(node_data->i_mode & S_IFREG)
  {
    inode = new RegularFile(id, sector, offset, fs_, volume_manager_, node_data->i_time, 0, 0, node_data->i_nlinks, node_data->i_size);
    debug(INODE_TABLE, "InodeTableMinix::createInodeFromDeviceData - File instance created\n");
  }
  // unknown / unsupported I-Node type
  else
  {
    debug(INODE_TABLE, "InodeTableMinix::createInodeFromDeviceData - unknown I-Node type=%x\n", node_data->i_mode);
    return NULL;
  }

  // init some minix related stuff
  initInode(inode, node_data->i_gid, node_data->i_uid, node_data->i_mode & 0x0FFF);

  // loading direct data sectors of I-Node
  for(uint16 i = 0; i < 7; i++)
  {
    if(node_data->i_zone[i] != UNUSED_DATA_BLOCK)
    {
      inode->addSector(node_data->i_zone[i]);
      debug(INODE_TABLE, "InodeTableMinix::createInodeFromDeviceData - added direct sector=%x\n", node_data->i_zone[i]);
    }
  }

  inode->setIndirectBlock(1, node_data->i_zone[7]);
  inode->setIndirectBlock(2, node_data->i_zone[8]);

  // resolve and load indirect data-sectors
  // i_zone[7] points to a single in-direct block and i_zone[8]
  // to a double-indirect block; all sector lens are 16bit (=2byte)
  fs_->resolveIndirectDataBlocks(inode, node_data->i_zone[7], 1, 2);
  fs_->resolveIndirectDataBlocks(inode, node_data->i_zone[8], 2, 2);

  return inode;
}

Inode* InodeTableMinix::getInode(inode_id_t id)
{
  debug(INODE_TABLE, "getInode - loading Inode %d\n", id);

  // 1. check if the I-Node with this ID is valid!
  if(inode_bitmap_.getBit(id-1) == false)
  {
    debug(INODE_TABLE, "getInode - Inode %d is not available\n", id);
    return NULL;
  }

  // 2. determine the I-Node's sector
  // read Sector and extract the I-Node
  sector_addr_t inode_sector = getInodeSectorAddr(id);
  sector_len_t inode_offset = getInodeSectorOffset(id);

  debug(INODE_TABLE, "getInode - reading inode from sector=%x offset=%d\n", inode_sector, inode_offset);

  volume_manager_->acquireSectorForReading(inode_sector);

  // reading the sector's data
  char* buffer = volume_manager_->readSectorUnprotected(inode_sector);

  // create and I-node instance from the obtained data!
  minix_inode* inode_data = reinterpret_cast<minix_inode*>(buffer + inode_offset);

  // debug-print
  printMinixInodeStruct(inode_data);

  // create the i-node with the obtained sector data
  Inode* inode = createInodeFromDeviceData(id, inode_sector, inode_offset, inode_data);

  // release sector
  volume_manager_->releaseReadSector(inode_sector);

  return inode;
}

inode_id_t InodeTableMinix::occupyAndReturnNextFreeInode(void)
{
  return inode_bitmap_.occupyNextFreeBit()+1;
}

bool InodeTableMinix::freeInode(inode_id_t id)
{
  // set i-node's bit to 0
  return inode_bitmap_.setBit(id - 1, false);
}

uint16 InodeTableMinix::convertInodeTypeToMinixMode(Inode::InodeType type)
{
  switch(type)
  {
    case Inode::InodeTypeFile:
      return S_IFREG;
    case Inode::InodeTypeDirectory:
      return S_IFDIR;
    case Inode::InodeTypeBlockDevice:
      return S_IFBLK;
    default:
      return 0; // unknown type
  }
  return 0;
}

void InodeTableMinix::storeDataBlocksToInode(Inode* inode, minix_inode* node_data)
{
  debug(INODE_TABLE, "storeDataBlocksToInode - going to store %d\n", inode->getID());

  // first seven sectors are directly addressed
  for(uint16 i = 0; i < 7; i++)
  {
    node_data->i_zone[i] = inode->getSector(i);
    debug(INODE_TABLE, "storeDataBlocksToInode - storing sector no=%d %x\n", i, node_data->i_zone[i]);
  }

  node_data->i_zone[7] = inode->getIndirectBlock(1);
  node_data->i_zone[8] = inode->getIndirectBlock(2);

  // calculate the number of the first sectors in indirect-addressing
  sector_addr_t sgl_first_sector = 7;
  sector_addr_t dbl_first_sector = 7 + fs_->getDataBlockSize() / MINIX_DATA_BLOCK_ADDR_LEN;

  // update the data blocks of the i-node
  sector_addr_t sgl_indirect = fs_->storeIndirectDataBlocks(inode, sgl_first_sector, node_data->i_zone[7], 1, MINIX_DATA_BLOCK_ADDR_LEN);
  sector_addr_t dbl_indirect = fs_->storeIndirectDataBlocks(inode, dbl_first_sector, node_data->i_zone[8], 2, MINIX_DATA_BLOCK_ADDR_LEN);

  assert( sgl_indirect != (sector_addr_t)-1U && dbl_indirect != (sector_addr_t)-1U );

  node_data->i_zone[7] = sgl_indirect;
  node_data->i_zone[8] = dbl_indirect;

  inode->setIndirectBlock(1, sgl_indirect);
  inode->setIndirectBlock(2, dbl_indirect);
}

bool InodeTableMinix::storeInode(inode_id_t id, Inode* inode)
{
  debug(INODE_TABLE, "storeInode - CALL Inode id=%d (%d)\n", id, inode->getID());

  if(id != inode->getID())
  {
    debug(INODE_TABLE, "storeInode - ERROR Inode (ID=%d) does not fit with ID=%d\n", inode->getID(), id);
    return false;
  }

  // check if Bit in InodeBitmap is set
  if(inode_bitmap_.getBit(id-1) == false)
  {
    debug(INODE_TABLE, "storeInode - ERROR InodeBitmap bit %d is not set!\n", id);
    assert(false); // indicates a consistency problem
    return false;
  }

  // 2. determine the I-Node's sector
  //sector_addr_t inodes_per_sector = volume_manager_->getBlockSize() / MINIX_INODE_SIZE;

  sector_addr_t inode_sector = inode->getDeviceSector();
  sector_len_t inode_offset = inode->getSectorOffset();

  debug(INODE_TABLE, "storeInode - read from %x offset=%x!\n", inode_sector, inode_offset);

  volume_manager_->acquireSectorForWriting(inode_sector);

  // reading the sector's data
  char* buffer = volume_manager_->readSectorUnprotected(inode_sector);

  if(buffer == NULL)
  {
    debug(INODE_TABLE, "storeInode - ERROR failed to read table sector %x!\n", inode_sector);
    volume_manager_->releaseWriteSector(inode_sector);
    return false;
  }

  // create and I-node instance from the obtained data!
  minix_inode* inode_data = reinterpret_cast<minix_inode*>(buffer + inode_offset);

  // update minix-i-node on the sector with the i-node instance data
  inode_data->i_mode = convertInodeTypeToMinixMode(inode->getType()) + inode->getPermissions();
  inode_data->i_nlinks = inode->getReferenceCount();
  inode_data->i_size = inode->getFileSize();
  inode_data->i_time = inode->getModTime();

  inode_data->i_uid = inode->getUID();
  inode_data->i_gid = inode->getGID();

  // store the data-blocks to the disk-inode
  storeDataBlocksToInode(inode, inode_data);

  // some debug-printing concerning the minix-inode struct
  printMinixInodeStruct(inode_data);

  // write updated sector data back to the Device
  volume_manager_->writeSectorUnprotected(inode_sector, buffer);

  // release sector
  volume_manager_->releaseWriteSector(inode_sector);

  // 3.

  debug(INODE_TABLE, "storeInode - DONE!\n");
  return true;
}

void InodeTableMinix::printMinixInodeStruct(minix_inode* m_inode)
{
  debug(INODE_TABLE, "printMinixInodeStruct - CALL %x!\n", m_inode);

  debug(INODE_TABLE, " i_mode=%o\n", m_inode->i_mode);
  debug(INODE_TABLE, " i_uid=%d\n", m_inode->i_uid);
  debug(INODE_TABLE, " i_size=%d\n", m_inode->i_size);
  debug(INODE_TABLE, " i_time=%d\n", m_inode->i_time);
  debug(INODE_TABLE, " i_gid=%d\n", m_inode->i_gid);
  debug(INODE_TABLE, " i_nlinks=%d\n", m_inode->i_nlinks);

  for(uint16 i = 0; i < 9; i++)
  {
    debug(INODE_TABLE, " i_zone[%d]=%x\n", i, m_inode->i_zone[i]);
  }

  debug(INODE_TABLE, "printMinixInodeStruct - DONE!\n");
}

sector_addr_t InodeTableMinix::getInodeSectorAddr(inode_id_t id)
{
  sector_len_t inodes_per_sector = volume_manager_->getBlockSize() / MINIX_INODE_SIZE;
  return inode_tbl_start_ + (id-1) / inodes_per_sector;
}

sector_len_t InodeTableMinix::getInodeSectorOffset(inode_id_t id)
{
  sector_len_t inodes_per_sector = volume_manager_->getBlockSize() / MINIX_INODE_SIZE;
  return ((id-1) % inodes_per_sector) * MINIX_INODE_SIZE;
}

void InodeTableMinix::initInode(Inode* inode, uint32 gid, uint32 uid, uint32 permissions)
{
  if(inode == NULL)
    return;

  // setting indirect data-blocks
  inode->setIndirectBlock(1, 0x00);
  inode->setIndirectBlock(2, 0x00);

  if(gid != 0) inode->setGID(gid);
  if(uid != 0) inode->setUID(uid);

  inode->setPermissions(permissions);
}
