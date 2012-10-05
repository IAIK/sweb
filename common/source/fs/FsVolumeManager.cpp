/**
 * Filename: FsVolumeManager.cpp
 * Description:
 *
 * Created on: 11.08.2012
 * Author: chris
 */

#include "fs/FsVolumeManager.h"

#include "fs/FileSystem.h"
#include "fs/DeviceCache.h"
#include "fs/device/FsDevice.h"

#ifdef USE_FILE_SYSTEM_ON_GUEST_OS
#include <cstring>
#include <string.h>
#endif

FsVolumeManager::FsVolumeManager(FileSystem* file_system, FsDevice* device,
    Cache::CacheFactory* cache_factory, Cache::num_items_t cache_max_elements) :
    file_system_(file_system), device_(device), sector_lock_manager_(),
    dev_sector_cache_(NULL)
{
  debug(VOLUME_MANAGER, "FsVolumeManager() - constructor\n");

  // create volume's sector cache with the help of the passed factory
  dev_sector_cache_ = cache_factory->getNewCache(device, cache_max_elements);

  debug(VOLUME_MANAGER, "FsVolumeManager() - created the cache with (%d) items\n", cache_max_elements);
}

FsVolumeManager::~FsVolumeManager()
{
  debug(VOLUME_MANAGER, "~FsVolumeManager() - \n");

  if(dev_sector_cache_ != NULL)
  {
    delete dev_sector_cache_;
  }
}

sector_len_t FsVolumeManager::getBlockSize(void) const
{
  return file_system_->getBlockSize();
}

sector_len_t FsVolumeManager::getDataBlockSize(void) const
{
  return file_system_->getDataBlockSize();
}

void FsVolumeManager::acquireSectorForReading(sector_addr_t sector)
{
  debug(VOLUME_MANAGER, "acquireSectorForReading - sector (%d)\n", sector);
  sector_lock_manager_.acquireReadBlocking(sector);
  sector_lock_manager_.setSlotAdditionalInfo(sector, 0);
}

void FsVolumeManager::acquireSectorForWriting(sector_addr_t sector)
{
  debug(VOLUME_MANAGER, "acquireSectorForWriting - sector (%d)\n", sector);
  sector_lock_manager_.acquireWriteBlocking(sector);
  sector_lock_manager_.setSlotAdditionalInfo(sector, 0);
}

void FsVolumeManager::releaseReadSector(sector_addr_t sector)
{
  debug(VOLUME_MANAGER, "releaseReadSector - sector (%d)\n", sector);

  // free all acquired cache references and release the slot
  decrRefCounterOfCacheElements(sector);
  sector_lock_manager_.releaseRead(sector);

  debug(VOLUME_MANAGER, "releaseReadSector - slot (%d) released\n", sector);
}

void FsVolumeManager::releaseWriteSector(sector_addr_t sector)
{
  debug(VOLUME_MANAGER, "releaseWriteSector - sector (%x)\n", sector);

  // free all acquired cache references and release the slot
  decrRefCounterOfCacheElements(sector);
  sector_lock_manager_.releaseWrite(sector);

  debug(VOLUME_MANAGER, "releaseWriteSector - slot (%d) released\n", sector);
}

void FsVolumeManager::decrRefCounterOfCacheElements(sector_addr_t sector)
{
  if(sector_lock_manager_.getSlotAdditionalInfo(sector) > 0)
  {
    SectorCacheIdent ident(sector, getBlockSize());

    for(uint32 i = 0; i < sector_lock_manager_.getSlotAdditionalInfo(sector); i++)
    {
      dev_sector_cache_->releaseItem(ident);
      debug(VOLUME_MANAGER, "release - cache item (%x) released\n", sector);
    }
  }
}

char* FsVolumeManager::readSectorUnprotected(sector_addr_t sector)
{
  debug(VOLUME_MANAGER, "readSectorUnprotected - reading sector=%x\n", sector);

  // Cache is available, try to get the Sector data from the Cache
  SectorCacheIdent ident(sector, getBlockSize());
  Cache::Item* item = dev_sector_cache_->getItem(ident);

  if(item == NULL)
  {
    kprintfd("FsVolumeManager::readSectorUnprotected I/O read error (sector=%x)\n", sector);
    return NULL;
  }

  debug(VOLUME_MANAGER, "readSectorUnprotected - return cache item\n");

  sector_lock_manager_.setSlotAdditionalInfo(sector, sector_lock_manager_.getSlotAdditionalInfo(sector) + 1);

  // item is valid, but no data in the item?!?
  assert(item->getData() != NULL);
  return reinterpret_cast<char*>(item->getData());

/*
  // read-out the Sector from the underlying device
  if(dev_sector_cache_ != NULL)
  {
    // Cache is available, try to get the Sector data from the Cache
    SectorCacheIdent ident(sector, getBlockSize());
    Cache::Item* item = dev_sector_cache_->getItem(ident);

    if(item != NULL)
    {
      debug(VOLUME_MANAGER, "readSectorUnprotected - return cache item\n");
      // item is valid, but no data in the item?!?
      assert(item->getData() != NULL);
      return reinterpret_cast<char*>(item->getData());
    }
  }

  // no Cache available, or sector item was not cached, so read from
  // the FileSystem's device
  sector_len_t sector_size = getBlockSize();

  char* sector_data = new char[sector_size];
  if(!device_->readSector(sector, sector_data, sector_size))
  {
    // FAIL, Sector could not be loaded!
    debug(VOLUME_MANAGER, "readSectorUnprotected - failed to read out sector!\n");
    return NULL;
  }
  debug(VOLUME_MANAGER, "readSectorUnprotected - sector read manually!\n");

  // if a Cache is available, the new Item will be inserted to it
  if(dev_sector_cache_ != NULL)
  {
    SectorCacheIdent ident(sector, getBlockSize());
    SectorCacheItem* item = new SectorCacheItem(sector_data);

    // insert new cache item
    dev_sector_cache_->addItem(ident, item);
    debug(VOLUME_MANAGER, "readSectorUnprotected - added new cache item manually!\n");
  }

  return sector_data;*/
}

bool FsVolumeManager::writeSectorUnprotected(sector_addr_t sector, const char* buffer)
{
  debug(VOLUME_MANAGER, "writeSectorUnprotected - writing sector=%x\n", sector);

  // NOTE: buffer_cpy will be freed at calling the destructor of Cache::Item
  if(dev_sector_cache_ != NULL)
  {
    SectorCacheIdent ident(sector, getBlockSize());

    char* buffer_cpy = new char[getBlockSize()];
    memcpy(buffer_cpy, buffer, getBlockSize());

    Cache::Item* item = new SectorCacheItem(buffer_cpy);

    // make a deep copy of the given block and let the Cache delete the item
    // when it was successfully written!
    bool write_to_cache = dev_sector_cache_->writeItem(ident, item, true);

    if(write_to_cache)
    {
      return true;
    }
    else
    {
      kprintfd("FsVolumeManager::writeSectorUnprotected I/O write error (sector=%x)\n", sector);
    }
  }

  // no cache here or just a read-only cache, so writing has to be
  // done manually
  bool ret_val = device_->writeSector(sector, buffer, getBlockSize());
  return ret_val;
}

void FsVolumeManager::acquireDataBlockForReading(sector_addr_t data_block)
{
  if(getDataBlockSize() % getBlockSize() != 0)
    return;

  // how many sectors are giving one data-block?
  uint32 num_sectors_per_data_block = getDataBlockSize() / getBlockSize();

  for(uint32 i = 0; i < num_sectors_per_data_block; i++)
  {
    acquireSectorForReading(file_system_->convertDataBlockToSectorAddress(data_block) + i);
  }
}

void FsVolumeManager::acquireDataBlockForWriting(sector_addr_t data_block)
{
  if(getDataBlockSize() % getBlockSize() != 0)
    return;

  // how many sectors are giving one data-block?
  uint32 num_sectors_per_data_block = getDataBlockSize() / getBlockSize();

  for(uint32 i = 0; i < num_sectors_per_data_block; i++)
  {
    acquireSectorForWriting(file_system_->convertDataBlockToSectorAddress(data_block) + i);
  }
}

void FsVolumeManager::releaseReadDataBlock(sector_addr_t data_block)
{
  if(getDataBlockSize() % getBlockSize() != 0)
    return;

  // how many sectors are giving one data-block?
  uint32 num_sectors_per_data_block = getDataBlockSize() / getBlockSize();

  for(uint32 i = 0; i < num_sectors_per_data_block; i++)
  {
    releaseReadSector(file_system_->convertDataBlockToSectorAddress(data_block) + i);
  }
}

void FsVolumeManager::releaseWriteDataBlock(sector_addr_t data_block)
{
  if(getDataBlockSize() % getBlockSize() != 0)
    return;

  // how many sectors are giving one data-block?
  uint32 num_sectors_per_data_block = getDataBlockSize() / getBlockSize();

  for(uint32 i = 0; i < num_sectors_per_data_block; i++)
  {
    releaseWriteSector(file_system_->convertDataBlockToSectorAddress(data_block) + i);
  }
}

char* FsVolumeManager::readDataBlockUnprotected(sector_addr_t data_block)
{
  if(getDataBlockSize() % getBlockSize() != 0)
    return NULL;

  sector_len_t block_size = getBlockSize();

  // how many sectors are giving one data-block?
  uint32 num_sectors_per_data_block = getDataBlockSize() / block_size;

  // the buffer holding the read data
  char* buffer = new char[getDataBlockSize()];

  for(uint32 i = 0; i < num_sectors_per_data_block; i++)
  {
    char* temp = readSectorUnprotected( file_system_->convertDataBlockToSectorAddress(data_block) + i );
    memcpy(buffer + (i*block_size), temp, block_size);
  }

  return buffer;
}

void FsVolumeManager::updateSectorData(sector_addr_t sector, const char* block, sector_len_t offset)
{
  // 1. update item in cache (synchronize cache)
  // Cache is available, try to get the Sector data from the Cache
  SectorCacheIdent ident(sector, getBlockSize());
  Cache::Item* item = dev_sector_cache_->getItem(ident);

  if(item == NULL)
  {
    return;
  }

  char* temp = reinterpret_cast<char*>(item->getData());
  memcpy(temp, block + (offset*getBlockSize()), getBlockSize());

  dev_sector_cache_->releaseItem(ident);
}

bool FsVolumeManager::writeDataBlockUnprotected(sector_addr_t data_block, const char* block)
{
  if(getDataBlockSize() % getBlockSize() != 0)
    return false;

  sector_len_t block_size = getBlockSize();

  // how many sectors are giving one data-block?
  uint32 num_sectors_per_data_block = getDataBlockSize() / block_size;

  for(uint32 i = 0; i < num_sectors_per_data_block; i++)
  {
    // calculate the address of the current sector
    sector_addr_t cur_sector = file_system_->convertDataBlockToSectorAddress(data_block) + i;

    updateSectorData(cur_sector, block, i);
/*
    // 1. update item in cache (synchronize cache)
    char* temp = readSectorUnprotected(cur_sector);
    if(temp == NULL)
    {
      return false;
    }
    memcpy(temp, block + (i*block_size), block_size);
*/
    // 2. save (write) to disk
    if(!writeSectorUnprotected(cur_sector, block + (i*block_size)))
    {
      return false;
    }
  }

  return true;
}

void FsVolumeManager::flush(void)
{
  dev_sector_cache_->flush();
}

bool FsVolumeManager::synchronizeSector(sector_addr_t sector)
{
  SectorCacheIdent ident(sector, getBlockSize());

  return dev_sector_cache_->writeItemImmediately(ident);
}

void FsVolumeManager::getCacheStat(Cache::CacheStat& stats)
{
  dev_sector_cache_->getStats(stats);
}
