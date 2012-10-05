/**
 * Filename: FileSystemMinix.cpp
 * Description:
 *
 * Created on: 16.05.2012
 * Author: chris
 */

#include "fs/minix/FileSystemMinix.h"
#include "fs/unixfs/UnixInodeCache.h"
#include "fs/minix/InodeTableMinix.h"
#include "fs/minix/InodeTableMinixV2.h"
#include "fs/minix/MinixDefs.h"
#include "fs/FsVolumeManager.h"
#include "fs/FsDefinitions.h"
#include "fs/device/FsDevice.h"

#include "fs/inodes/Directory.h"
#include "fs/inodes/RegularFile.h"
#include "fs/Statfs.h"

#include "fs/VfsSyscall.h"

#ifdef USE_FILE_SYSTEM_ON_GUEST_OS
#include <cstring>
#endif

FileSystemMinix::FileSystemMinix(FsDevice* device, uint32 mount_flags,
    minix_super_block super_block, uint16 minix_version, uint16 filename_len) :
    FileSystemUnix(device, mount_flags),
    superblock_(super_block), zone_size_(0),
    FILENAME_LEN_(filename_len), DIR_ENTRY_SIZE_(FILENAME_LEN_+2),
    zone_bitmap_(NULL)
{
  assert( FILENAME_LEN_ == 14 || FILENAME_LEN_ == 30 );

  // setting Minix Blocks size on FsDevice
  device_->setBlockSize(getBlockSize());

  // calculate the bitmap and table offsets
  calculateOffsets();

  // creating the i-node table
  createInodeTable(minix_version);

  // create the zone-bitmap
  zone_bitmap_ = new FsBitmap(this, this->getVolumeManager(),
                              zone_bitmap_sector_, zone_bitmap_sector_+zone_bitmap_size_-1,
                              superblock_.s_nzones);
  debug(FS_MINIX, "FileSystemMinix() - FsBitmap Zone successfully created\n");

  // init root-directory
  initRootInode();
}

FileSystemMinix::~FileSystemMinix()
{
  // 1. release root-inode (is the last Inode that was released)
  releaseInode(root_);
  debug(FS_MINIX, "~FileSystemMinix() - Root Inode was succsessfully removed\n");

  // 2. now flush the inode-cache and then destroy the InodeTable
  if(inode_table_ != NULL)
  {
    // flush inode-cache
    inode_cache_->flush();
    delete inode_table_;
    inode_table_ = NULL;
  }

  // 3. delete zone-bitmap (at now on this Minix instance can be considered as dead!)
  if(zone_bitmap_ != NULL)
  {
    delete zone_bitmap_;
  }
}

bool FileSystemMinix::readSuperblock(FsDevice* device, minix_super_block& superblock)
{
  sector_addr_t sector = MINIX_SUPERBLOCK_OFFSET / MINIX_BLOCK_SIZE;
  sector_len_t offset = MINIX_SUPERBLOCK_OFFSET % MINIX_BLOCK_SIZE;

  debug(FS_MINIX, "readSuperblock - reading Superblock from %x, offset=%x\n", sector, offset);

  // the buffer containing the data of the MINIX_SUPERBLOCK_OFFSET
  char* buffer = new char[MINIX_BLOCK_SIZE];

  if(!device->readSector(sector, buffer, MINIX_BLOCK_SIZE))
  {
    debug(FS_MINIX, "readSuperblock - I/O read error, failed to read SB!\n");

    delete[] buffer;
    return false;
  }

  // copy of Super-block data
  superblock = *reinterpret_cast<minix_super_block*>(buffer + offset);

  // buffer is no longer used!
  delete[] buffer;
  buffer = NULL;

  // some debug prints
  debug(FS_MINIX, "readSuperblock - magic-number=%x\n", superblock.s_magic);
  debug(FS_MINIX, "readSuperblock - number of i-nodes=%d\n", superblock.s_ninodes);
  debug(FS_MINIX, "readSuperblock - first data zone=%x\n", superblock.s_firstdatazone);
  debug(FS_MINIX, "readSuperblock - number of zones=%d\n", superblock.s_nzones);
  debug(FS_MINIX, "readSuperblock - last mount state=%d\n", superblock.s_state);

  // check the Signature of the Superblock
  if( superblock.s_magic != MINIX_SUPER_MAGIC && superblock.s_magic != MINIX_SUPER_MAGIC2
      && superblock.s_magic != MINIX2_SUPER_MAGIC && superblock.s_magic != MINIX2_SUPER_MAGIC2
      && superblock.s_magic != MINIX3_SUPER_MAGIC )
  {
    debug(FS_MINIX, "readSuperblock - no correct minix Signature recognized!\n");
    return false;
  }

  // make some checks concerning validity of Super-block data

  // pre-calculate some values:
  //zone_size_ = static_cast<sector_len_t>(MINIX_BLOCK_SIZE << superblock_.s_log_zone_size);

  return true;
}

void FileSystemMinix::calculateOffsets(void)
{
  // calculate the bitmap and table offsets

  // the InodeTable is one data-block behind the Superblock
  inode_bitmap_sector_ = (MINIX_SUPERBLOCK_OFFSET / getBlockSize()) + 1;
  inode_bitmap_size_ = (superblock_.s_ninodes) / (getBlockSize()*8);
  if((superblock_.s_ninodes) % (getBlockSize()*8) > 0)
  {
    inode_bitmap_size_++;
  }
  assert(superblock_.s_imap_blocks == inode_bitmap_size_);
  debug(FS_MINIX, "calculateOffsets - Inode-Bitmap Offset=%d Size=%d\n", inode_bitmap_sector_, inode_bitmap_size_);

  zone_bitmap_sector_ = inode_bitmap_sector_ + inode_bitmap_size_;
  zone_bitmap_size_ = superblock_.s_nzones / (getBlockSize()*8);
  if((superblock_.s_nzones) % (getBlockSize()*8) > 0)
  {
    zone_bitmap_size_++;
  }
  assert(superblock_.s_zmap_blocks == zone_bitmap_size_);
  debug(FS_MINIX, "calculateOffsets - Zone-Bitmap Offset=%d Size=%d\n", zone_bitmap_sector_, zone_bitmap_size_);

  inode_tbl_sector_ = zone_bitmap_sector_ + zone_bitmap_size_;
  inode_tbl_size_ = (superblock_.s_ninodes * MINIX_INODE_SIZE) / getBlockSize();
  if((superblock_.s_ninodes * MINIX_INODE_SIZE) % getBlockSize() > 0)
  {
    inode_tbl_size_++;
  }
  debug(FS_MINIX, "Inode-Table Offset=%d Size=%d\n", inode_tbl_sector_, inode_tbl_size_);

  // check if the all calculations are correct, by computing the address of the first
  // data zone and comparing the value to the one stored in the superblock
  sector_addr_t first_data_zone = inode_tbl_sector_ + inode_tbl_size_;

  debug(FS_MINIX, "First data zone = %d\n", first_data_zone);
  debug(FS_MINIX, "Superblock first data zone = %d\n", superblock_.s_firstdatazone);
  assert(superblock_.s_firstdatazone >= first_data_zone); // TODO error handling

  // calculate the zone-size
  zone_size_ = static_cast<sector_len_t>(MINIX_BLOCK_SIZE << superblock_.s_log_zone_size);
}

void FileSystemMinix::createInodeTable(uint16 minix_version)
{
  debug(FS_MINIX, "FileSystemMinix() - creating InodeTableMinix\n");

  // Minix Version 2
  if(minix_version == 2)
  {
    inode_table_ = new InodeTableMinixV2(this, this->getVolumeManager(),
                                       inode_tbl_sector_, inode_tbl_size_,
                                       inode_bitmap_sector_, inode_bitmap_sector_+inode_bitmap_size_-1,
                                       superblock_.s_ninodes);
  }
  // default: Minix Version 1
  else
  {
    inode_table_ = new InodeTableMinix(this, this->getVolumeManager(),
                                       inode_tbl_sector_, inode_tbl_size_,
                                       inode_bitmap_sector_, inode_bitmap_sector_+inode_bitmap_size_-1,
                                       superblock_.s_ninodes);
  }

  // the InodeCache uses the InodeTable as Device (to load and store cached Inodes)
  inode_cache_->setCacheDevice(inode_table_);

  debug(FS_MINIX, "FileSystemMinix() - InodeTableMinix successfully created\n");
}

void FileSystemMinix::initRootInode(void)
{
  debug(FS_MINIX, "initRootInode - CALL\n");

  // getting the root-inode out of the InodeTable
  Inode* root_node = this->acquireInode(MINIX_ROOT_INO, NULL, "/");

  assert(root_node != NULL);
  assert(root_node->getID() == MINIX_ROOT_INO);

  if(root_node->getType() != Inode::InodeTypeDirectory)
  {
    // todo handle error case
    debug(FS_MINIX, "initRootInode - ERROR root i-node is not a Directory!\n");
    assert(false);
    return;
  }
  root_ = reinterpret_cast<Directory*>(root_node);

  debug(FS_MINIX, "initRootInode - root I-node was successfully loaded.\n");

  // load the root-s children
  this->loadDirChildrenUnsafe(root_);

  debug(FS_MINIX, "initRootInode - DONE\n");
}

uint8 FileSystemMinix::getPartitionIdentifier(void) const
{
  // Minix-Partition identifier
  return 0x81;
}

const char* FileSystemMinix::getName(void) const
{
  return "minix";
}

File* FileSystemMinix::creat(Directory* parent, const char* name,
                             mode_t permissions, uid_t uid, gid_t gid)
{
  // create a new file in the given parent directory
  debug(FS_MINIX, "creat - CALL parent ID=%d filename \"%s\"\n", parent->getID(), name);

  // check arguments
  if(strlen(name) > FILENAME_LEN_)
  {
    debug(FS_MINIX, "creat - ERROR filename too long; max %d chars allowed!\n", FILENAME_LEN_);
    return NULL;
  }

  // 1. obtain a free InodeID for the new File
  inode_id_t id = inode_table_->occupyAndReturnNextFreeInode();

  if(id == -1U)
  {
    debug(FS_MINIX, "creat - ERROR failed no more free i-nodes...\n");
    return NULL;
  }

  // current time-stamp
  unix_time_stamp current_time = VfsSyscall::getCurrentTimeStamp();

  // 2. create the i-node instance;
  // * id is the just obtained one
  // * sector and offset in the sector can be calculated from the id
  // * times are set to current time
  // * reference count is 1
  // * file-size is 0 (empty new file)
  RegularFile* new_file = new RegularFile(id, inode_table_->getInodeSectorAddr(id),
                                          inode_table_->getInodeSectorOffset(id),
                                          this, this->getVolumeManager(),
                                          current_time, current_time, current_time,
                                          1, 0);

  if(new_file == NULL)
  {
    debug(FS_MINIX, "creat - ERROR failed to create new file instance.\n");
    return NULL;
  }

  // apply default values for GID, UID und permissions
  inode_table_->initInode(new_file, gid, uid, permissions);

  // add a hard-link reference in the parent directory to the new file
  if(!addDirectoryEntry(parent, name, id))
  {
    debug(FS_MINIX, "creat - ERROR failed to create a hard link to the new file.\n");
    delete new_file;
    return NULL;
  }

  // store the new File in the I-Node cache and in the table:
  UnixInodeIdent ident(id);
  UnixInodeCacheItem* item = new UnixInodeCacheItem(new_file);

  inode_cache_->addItem(ident, item);
  inode_cache_->writeItem(ident, item);

  // perform a "dummy"-acquire to increase the reference counter of the new
  // created file
  if( inode_cache_->getItem(ident) != item )
  {
    debug(FS_MINIX, "creat - ERROR failed to add I-Node correctly to Cache!\n");

    delete new_file;
    return NULL;
  }

  debug(FS_MINIX, "creat - DONE new file successfully created.\n");
  //debug(FS_MINIX, "creat - File-id=%d Sector=%x Offset=%d.\n", id, inode_sector, inode_offset);

  return new_file;
}

int32 FileSystemMinix::mkdir(Directory* parent, const char* name,
                             unix_time_stamp current_time,
                             mode_t permissions, uid_t uid, gid_t gid,
                             inode_id_t& new_dir_id)
{
  debug(FS_MINIX, "mkdir - CALL parent ID=%d name=\"%s\"\n", parent->getID(), name);

  // check argument
  if(strlen(name) > FILENAME_LEN_)
  {
    debug(FS_MINIX, "mkdir - ERROR filename too long; max %d chars allowed!\n", FILENAME_LEN_);
    return -1;
  }

  // 1. create a new Directory in the given parent directory
  inode_id_t id = inode_table_->occupyAndReturnNextFreeInode();

  if(id == -1U)
  {
    debug(FS_MINIX, "mkdir - ERROR no more free i-nodes, FS is full!\n");
    return -1; // TODO error-code
  }

  debug(FS_MINIX, "mkdir - Inode ID for new Directory (%d).\n", id);

  // 2. create the i-node instance;
  // * id is the just obtained one
  // * sector and offset in the sector can be calculated from the id
  // * times are set to current time
  Directory* new_directory = new Directory(id, inode_table_->getInodeSectorAddr(id),
                                           inode_table_->getInodeSectorOffset(id),
                                          this,
                                          current_time, current_time, current_time);

  if(new_directory == NULL)
  {
    debug(FS_MINIX, "mkdir - ERROR failed to create new Directory instance.\n");
    return -1;
  }

  // apply default values for GID, UID und permissions
  inode_table_->initInode(new_directory, gid, uid, permissions);

  // add the self reference "." and the parent reference ".."
  if(!addDirectoryEntry(new_directory, ".", id) ||
     !addDirectoryEntry(new_directory, "..", parent->getID()))
  {
    debug(FS_MINIX, "mkdir - ERROR failed to create \".\" or \"..\".\n");

    inode_table_->freeInode(id);
    delete new_directory;
    return -1;
  }

  new_directory->addChild(".", id);
  new_directory->addChild("..", parent->getID());
  new_directory->allChildrenLoaded();

  // 3. add a hard-link reference in the parent directory to the new file
  if(!addDirectoryEntry(parent, name, id))
  {
    debug(FS_MINIX, "mkdir - ERROR failed to create the hard-link reference in the parent directory.\n");

    inode_table_->freeInode(id);
    delete new_directory;
    return -1;
  }

  // store the new File in the I-Node cache and in the table:
  UnixInodeIdent ident(id);
  UnixInodeCacheItem* item = new UnixInodeCacheItem(new_directory);
  debug(FS_MINIX, "mkdir - creating CacheItem holding the Inode %x.\n", item);

  inode_cache_->addItem(ident, item);
  if( !inode_cache_->writeItem(ident, item) )
  {
    debug(FS_MINIX, "mkdir - ERROR failed to write item to table!\n");

    inode_table_->freeInode(id);
    delete new_directory;
    return false;
  }

  debug(FS_MINIX, "mkdir - DONE new empty directory successfully created.\n");
  //debug(FS_MINIX, "mkdir - File-id=%d Sector=%x Offset=%d.\n", id, inode_sector, inode_offset);

  new_dir_id = id;
  return 0;
}

int32 FileSystemMinix::rmdir(Directory* parent, inode_id_t inode_id, const char* name)
{
  debug(FS_MINIX, "rmdir - CALL parent ID=%d Directory to remove %d \"%s\".\n", parent->getID(), inode_id, name);

  if(inode_id == MINIX_ROOT_INO)
  {
    debug(FS_MINIX, "rmdir - ERROR are you stupid?! You can NOT delete the root inode!\n");
    return -1;
  }

  // 1. remove hard link of directory
  if(!removeDirectoryEntry(parent, name, inode_id))
  {
    debug(FS_MINIX, "rmdir - ERROR failed to remove the given directory.\n");
    return -1;
  }

  // 2. remove Directory Inode-instance in memory and on the FileSystem's disk
  UnixInodeIdent ident(inode_id);
  inode_cache_->removeItem(ident);

  return 0;
}

int32 FileSystemMinix::link(File* file, Directory* parent, const char* name)
{
  if(file->getReferenceCount() >= MINIX_LINK_MAX)
  {
    debug(FS_MINIX, "link - ERROR max link (%d) is already reached, no more links to this file!\n", MINIX_LINK_MAX);
    return -1;
  }

  // create a new hard-link to an existing file (inode)
  if(!addDirectoryEntry(parent, name, file->getID()))
  {
    debug(FS_MINIX, "link - ERROR failed establish a new link.\n");
    return -1;
  }

  // increment reference counter of file
  file->incrReferenceCount();

  return 0;
}

int32 FileSystemMinix::unlink(File* file, Directory* parent, const char* ref_name)
{
  // 1. remove hard-link
  if( !removeDirectoryEntry(parent, ref_name, file->getID()) )
  {
    debug(FS_MINIX, "unlink - ERROR failed remove a hard-link.\n");
    return -1;
  }

  // 2. decrement link-counter
  file->decrReferenceCount();

  // 3. maybe delete i-node
  if(file->getReferenceCount() == 0)
  {
    UnixInodeIdent ident(file->getID());
    inode_cache_->removeItem(ident);
  }

  return 0;
}

bool FileSystemMinix::loadDirChildrenUnsafe(Directory* parent)
{
  debug(FS_MINIX, "loadDirChildrenUnsafe - going to load i-node %d's children\n", parent->getID());

  // clearing the Directorie's cache
  parent->clearAllChildren();

  // searching the entry of the i-node
  for(uint32 i = 0; parent->getSector(i) != 0; i++)
  {
    sector_addr_t cur_data_block = parent->getSector(i);
    debug(FS_MINIX, "loadDirChildrenUnsafe - read Directory sector=%x (%d th sector)\n", cur_data_block, i);

    // lock for writing
    volume_manager_->acquireDataBlockForReading(cur_data_block);

    // readout data-block
    char* block = volume_manager_->readDataBlockUnprotected(cur_data_block);

    debug(FS_MINIX, "sizeof(minix_dirent)=%d\n", sizeof(minix_dirent));

    // reading dirents
    for(sector_len_t i = 0; i < zone_size_; i+=DIR_ENTRY_SIZE_)
    {
      // add to cache
      minix_dirent* cur_entry = reinterpret_cast<minix_dirent*>(block + i);

      if(cur_entry->inode_no == 0x00/* && cur_entry->filename[0] == 0x00*/)
      {
        // seems to be an empty entry, skip it
        continue;
      }

      char* safe_filename = safeEscapeFilename(cur_entry->filename, FILENAME_LEN_);

      parent->addChild(safe_filename, cur_entry->inode_no);
      debug(FS_MINIX, "Dir-entry id=%d name=\"%s\"\n", cur_entry->inode_no, safe_filename);

      delete[] safe_filename;
    }

    delete[] block;

    // release data-block
    volume_manager_->releaseReadDataBlock(cur_data_block);
  }

  // cache is now valid again :)
  parent->allChildrenLoaded();

  return true;
}

Inode* FileSystemMinix::acquireInode(inode_id_t id,
                                     Directory* parent __attribute__((unused)),
                                     const char* name __attribute__((unused)))
{
  debug(FS_MINIX, "acquireInode - getting I-Node %d\n", id);

  // try to get the I-Node from the Cache
  if(inode_cache_ != NULL)
  {
    debug(FS_MINIX, "acquireInode - fetching I-Node %d from Cache\n", id);
    UnixInodeIdent ident(id);
    Cache::Item* item = inode_cache_->getItem(ident);

    if(item != NULL)
    {
      debug(FS_MINIX, "acquireInode - I-Node fetched successfully from/by Cache\n");

      // item was in cache:
      return reinterpret_cast<Inode*>(item->getData());
    }
  }

  debug(FS_MINIX, "acquireInode - I-Node was not in Cache - load manually\n");
  // read I-Node from the InodeTable
  return inode_table_->getInode(id);
}

void FileSystemMinix::releaseInode(Inode* inode)
{
  debug(FS_MINIX, "releaseInode - releasing I-Node %d\n", inode->getID());

  if(inode_cache_ != NULL)
  {
    UnixInodeIdent ident(inode->getID());
    inode_cache_->releaseItem(ident);
  }

  // nothing to-do here, if no Cache is in use
}

bool FileSystemMinix::writeInode(Inode* inode)
{
  // writing an I-Node back to the device...

  // write back by using the Cache
  if(inode_cache_ != NULL)
  {
    UnixInodeIdent ident(inode->getID());

    //Cache::Item* item = inode_cache_->getItem(ident);
    //assert(inode == item->getData());
    //UnixInodeCacheItem item(inode);

    // writing the I-Node to the WriteCache
    if(inode_cache_->writeItem(ident, NULL))
    {
      //inode_cache_->releaseItem(ident);
      return true;
    }
    //inode_cache_->releaseItem(ident);
  }

  // writing I-Node back by using the InodeTable:
  return inode_table_->storeInode(inode->getID(), inode);
}

bool FileSystemMinix::destroyInode(Inode* inode_ro_destroy)
{
  debug(FS_MINIX, "destroyInode - going to remove inode from the volume.\n");

  // free all used data-blocks
  for(sector_addr_t i = 0; inode_ro_destroy->getSector(i) != 0; i++)
  {
    sector_addr_t cur_sector = inode_ro_destroy->getSector(i);

    if(!freeOccupiedBlock(cur_sector))
    {
      debug(FS_MINIX, "destroyInode - failed to free data-block %x.\n", cur_sector);
      return false;
    }
  }

  // remove all sectors from the list
  inode_ro_destroy->clearSectorList();

  // remove possible existing indirect addressing helper blocks
  sector_addr_t sgl = storeIndirectDataBlocks(inode_ro_destroy, 7, inode_ro_destroy->getIndirectBlock(1), 1, 2);
  sector_addr_t dbl = storeIndirectDataBlocks(inode_ro_destroy, 7+512, inode_ro_destroy->getIndirectBlock(2), 2, 2);

  if(sgl != 0x00 || dbl != 0x00)
  {
    debug(FS_MINIX, "destroyInode - ERROR failed cleanup remaining indirect data-blocks.\n");
    assert(false); // TODO just temp
    return false;
  }

  return true;
}

sector_len_t FileSystemMinix::getBlockSize(void) const
{
  return MINIX_BLOCK_SIZE;
}

sector_len_t FileSystemMinix::getDataBlockSize(void) const
{
  return zone_size_;
}

sector_addr_t FileSystemMinix::convertDataBlockToSectorAddress(sector_addr_t data_block)
{
  // minix data-block addresses are equivalent to the sector-addresses
  // so just return the given value
  return data_block;
}

sector_addr_t FileSystemMinix::occupyAndReturnFreeBlock(bool clear_block)
{
  debug(FS_MINIX, "occupyAndReturnFreeBlock - CALL clear_block=%d\n", clear_block);

  bitmap_t next_free_zone = zone_bitmap_->occupyNextFreeBit();

  if(next_free_zone == -1U)
  {
    // no more free zones
    debug(FS_MINIX, "occupyAndReturnFreeBlock - no more free zones.\n");
    return 0;
  }

  // calculate the address of the new data-block
  sector_addr_t new_block_addr = getFirstDataBlockAddress() + next_free_zone;

  debug(FS_MINIX, "occupyAndReturnFreeBlock - new block is=%x.\n", new_block_addr);

  // clear the data-block; set all bytes to 0x00
  if(clear_block)
  {
    debug(FS_MINIX, "occupyAndReturnFreeBlock - resetting data-block.\n");

    char* block = new char[getDataBlockSize()];
    memset(block, 0x00, getDataBlockSize());

    volume_manager_->acquireDataBlockForWriting(new_block_addr);

    // since this is the first time (and for sure the ONLY thread to write) to
    // this data-block, no locking aids are required!
    volume_manager_->writeDataBlockUnprotected(new_block_addr, block);
    volume_manager_->releaseWriteDataBlock(new_block_addr);
    delete[] block;
  }

  return new_block_addr;
}

bool FileSystemMinix::freeOccupiedBlock(sector_addr_t block_address)
{
  debug(FS_MINIX, "freeOccupiedBlock - CALL freeing block=%x\n", block_address);

  if(block_address < getFirstDataBlockAddress())
  {
    debug(FS_MINIX, "freeOccupiedBlock - ERROR invalid data-block address given (%x)!\n", block_address);
    return false;
  }

  // convert the given block-address to the bit-number in the Zone Bitmap
  bitmap_t bit = block_address - getFirstDataBlockAddress();
/*
  if(zone_bitmap_->getBit(bit))
  {
    debug(FS_MINIX, "freeOccupiedBlock - ERROR bit is already freed!\n", block_address);
    return false;
  }
*/
  // set the bit to 0!
  bool result = zone_bitmap_->setBit(bit, false);

  debug(FS_MINIX, "freeOccupiedBlock - DONE with return value (%d).\n", result);
  return result;
}

sector_addr_t FileSystemMinix::getFirstDataBlockAddress(void) const
{
  return superblock_.s_firstdatazone;
}

sector_addr_t FileSystemMinix::appendSectorToInode(Inode* inode, bool zero_out_sector)
{
  debug(FS_MINIX, "appendSectorToInode - CALL InodeID=%d\n", inode->getID());

  sector_addr_t next_free_zone = occupyAndReturnFreeBlock(zero_out_sector);

  if(next_free_zone == 0)
  {
    debug(FS_MINIX, "appendSectorToInode - ERROR failed, no more free data-blocks!\n");
    return 0;
  }

  inode->addSector(next_free_zone);

  debug(FS_MINIX, "appendSectorToInode - DONE!\n");
  return next_free_zone;
}

sector_addr_t FileSystemMinix::removeSectorFromInode(Inode* inode, uint32 sector_to_remove)
{
  debug(FS_MINIX, "removeSectorFromInode - CALL InodeID=%d\n", inode->getID());

  // getting address of Inode's last sector
  sector_addr_t inode_last_sector = inode->getSector( sector_to_remove );

  if(inode_last_sector == 0)
  {
    debug(FS_MINIX, "removeSectorFromInode - ERROR non such a sector in the I-Node's list!\n");
    return 0;
  }

  // free it
  if(!freeOccupiedBlock(inode_last_sector))
  {
    debug(FS_MINIX, "removeSectorFromInode - ERROR failed to remove sector!\n");
    return 0;
  }

  inode->removeSector(sector_to_remove);
  debug(FS_MINIX, "removeSectorFromInode - DONE!\n");

  return inode_last_sector;
}

sector_addr_t FileSystemMinix::removeLastSectorOfInode(Inode* inode)
{
  debug(FS_MINIX, "removeLastSectorOfInode - CALL InodeID=%d\n", inode->getID());

  // getting address of Inode's last sector
  sector_addr_t inode_last_sector = inode->getSector( inode->getNumSectors() - 1 );

  // free it
  if(!freeOccupiedBlock(inode_last_sector))
  {
    debug(FS_MINIX, "removeLastSectorOfInode - ERROR failed to remove last sector!\n");
    return 0;
  }

  inode->removeLastSector();
  debug(FS_MINIX, "removeLastSectorOfInode - DONE!\n");

  return inode_last_sector;
}

void FileSystemMinix::updateInodesSectorList(Inode* inode __attribute__((unused)))
{
  // already fully loaded initially, so here is nothing left to do
}

bool FileSystemMinix::addDirectoryEntry(Directory* parent, const char* name, uint16 inode)
{
  assert(this == parent->getFileSystem());
  if(this != parent->getFileSystem())
    return false;

  debug(FS_MINIX, "addDirectoryEntry - adding a hard-link reference pointing to \"%s\" - %d\n", name, inode);

  bool done = false;

  // searching for a free entry
  for(uint32 j = 0; parent->getSector(j) != 0; j++)
  {
    sector_addr_t cur_data_block = parent->getSector(j);

    debug(FS_MINIX, "addDirectoryEntry - (%d) sector=%x\n", j, cur_data_block);

    // lock for writing
    volume_manager_->acquireDataBlockForWriting(cur_data_block);

    // readout data-block
    char* block = volume_manager_->readDataBlockUnprotected(cur_data_block);

    if(block == NULL)
    {
      // I/O read error!
      volume_manager_->releaseWriteDataBlock(cur_data_block);
      return false;
    }

    bool ret_val = false;

    // reading dirents
    for(sector_len_t i = 0; i < zone_size_; i+=DIR_ENTRY_SIZE_)
    {
      if(block[i] == 0x00 && block[i+1] == 0x00
          /*&& block[i+2] == 0x00*/)
      {
        debug(FS_MINIX, "addDirectoryEntry - adding new hard-link at %x offset=%x.\n", cur_data_block, i);

        // entry is empty, place data here:
        addDirEntryToBuffer(block, i, inode, name);

        // a new entry is appended to the data-zone of the parent-directory
        if(j*zone_size_ + (i+1)*DIR_ENTRY_SIZE_ > parent->getFileSize())
        {
          // so the file-size gets bigger
          debug(FS_MINIX, "addDirectoryEntry - increase Directory's size by %d (old size %d).\n", DIR_ENTRY_SIZE_, parent->getFileSize());
          parent->setFileSize( parent->getFileSize() + DIR_ENTRY_SIZE_ );
        }

        // store changed block on disk and quit
        ret_val = volume_manager_->writeDataBlockUnprotected(cur_data_block, block);

        debug(FS_MINIX, "addDirectoryEntry - new directory entry added.\n");
        done = true;
        break;
      }
    }

    delete[] block;
    // release data-block
    volume_manager_->releaseWriteDataBlock(cur_data_block);

    if(done)
    {
      return ret_val;
    }
  }

  debug(FS_MINIX, "addDirectoryEntry - no free space for the new entry found, add data-block.\n");

  // no place found for the new entry, add a new sector to the directory
  sector_addr_t new_sector = appendSectorToInode(parent, true);

  if(new_sector == 0)
  {
    // out of sectors, fail
    return false;
  }

  volume_manager_->acquireDataBlockForWriting(new_sector);

  // add entry to sector (no locking needed since the sector is new)
  char* buffer = volume_manager_->readDataBlockUnprotected(new_sector);

  addDirEntryToBuffer(buffer, 0x00, inode, name);

  // increase directory size
  parent->setFileSize( parent->getFileSize() + DIR_ENTRY_SIZE_ );

  // write updated buffer to disk
  volume_manager_->writeDataBlockUnprotected(new_sector, buffer);

  volume_manager_->releaseWriteDataBlock(new_sector);
  delete[] buffer;

  return true;
}

bool FileSystemMinix::removeDirectoryEntry(Directory* parent, const char* name, uint16 inode)
{
  // searching the entry of the i-node
  for(uint32 j = 0; parent->getSector(j) != 0; j++)
  {
    sector_addr_t cur_data_block = parent->getSector(j);

    // lock for writing
    volume_manager_->acquireDataBlockForWriting(cur_data_block);

    // readout data-block
    char* block = volume_manager_->readDataBlockUnprotected(cur_data_block);

    // reading dirents
    for(sector_len_t i = 0; i < zone_size_; i+=DIR_ENTRY_SIZE_)
    {
      // add to cache
      minix_dirent* cur_entry = reinterpret_cast<minix_dirent*>(block + i);

      if(cur_entry->inode_no == inode)
      {
        debug(FS_MINIX, "removeDirectoryEntry - remove entry for inode %d\n", cur_entry->inode_no);

        // consistency check, compare filenames
        assert( strcmp(name, cur_entry->filename) == 0 );

        // remove current entry
        memset(block + i, 0x00, DIR_ENTRY_SIZE_);

        // perform a check if the current handled data-block is now completely
        // free of references ...
        if(isDataBlockFreeOfReferences(block, getDataBlockSize(), DIR_ENTRY_SIZE_))
        {
          // ... if so, it is not used any longer and can therefore be freed
          //this->removeLastSectorOfInode(parent);
          removeSectorFromInode(parent, j);

          assert(parent->getFileSize() >= getDataBlockSize());
          parent->setFileSize( parent->getFileSize() - getDataBlockSize() );

          // Adapt the index
          j--; // TODO underflow problem!

          debug(FS_MINIX, "removeDirectoryEntry - data-block unused, so remove it!\n");
        }
        // if this entry is located at the end of all entries the file-size
        // will be decreased
        else if(j*zone_size_ + i+DIR_ENTRY_SIZE_ == parent->getFileSize())
        {
          debug(FS_MINIX, "removeDirectoryEntry - decrease Directory's size by %d.\n", DIR_ENTRY_SIZE_);
          parent->setFileSize( parent->getFileSize() - DIR_ENTRY_SIZE_ );
        }

        // write data block back to disk
        volume_manager_->writeDataBlockUnprotected(cur_data_block, block);

        volume_manager_->releaseWriteDataBlock(cur_data_block);
        delete[] block;
        return true;
      }
    }
    delete[] block;

    // release data-block
    volume_manager_->releaseWriteDataBlock(cur_data_block);
  }

  return false;
}

void FileSystemMinix::addDirEntryToBuffer(char* buffer, sector_len_t buffer_offset,
                           uint16 inode, const char* name)
{
  debug(FS_MINIX, "addDirEntryToBuffer - CALL InodeID=%d name=\"%s\"\n", inode, name);

  // copy inode-id
  memcpy(buffer + buffer_offset, &inode, 2);

  // determine length of given name
  uint16 name_len = strlen(name);
  if(name_len > FILENAME_LEN_)
  {
    name_len = FILENAME_LEN_;
  }

  memcpy(buffer + buffer_offset + 2, name, name_len);
}

bool FileSystemMinix::isDataBlockFreeOfReferences(const char* buffer, sector_len_t buf_len, uint16 directory_entry_len)
{
  for(sector_len_t pos = 0; pos < buf_len; pos += directory_entry_len)
  {
    if(buffer[pos + 0x00] != 0x00 || buffer[pos + 0x01] != 0x00
        || buffer[pos + 0x02] != 0x00)
    {
      return false;
    }
  }

  return true;
}

statfs_s* FileSystemMinix::statfs(void) const
{
  statfs_s* stat_info = new statfs_s;

  stat_info->fs_ident = this->getPartitionIdentifier();

  stat_info->block_size = this->getBlockSize();
  stat_info->num_blocks = superblock_.s_nzones; //device_->getNumBlocks();
  stat_info->num_free_blocks = zone_bitmap_->getNumFreeBits();

  stat_info->max_inodes = superblock_.s_ninodes;
  stat_info->used_inodes = superblock_.s_ninodes - inode_table_->getNumFreeInodes();

  volume_manager_->getCacheStat(stat_info->dev_cache_stat);
  inode_cache_->getStats(stat_info->inode_cache_stat);

  return stat_info;
}
