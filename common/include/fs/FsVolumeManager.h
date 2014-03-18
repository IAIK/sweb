/**
 * Filename: FsVolumeManager.h
 * Description:
 *
 * Created on: 11.08.2012
 * Author: chris
 */

#ifndef FSVOLUMEMANAGER_H_
#define FSVOLUMEMANAGER_H_

class FsDevice;
class FileSystem;

#include "cache/GeneralCache.h"
#include "cache/CacheFactory.h"
#include "util/SlotLockManager.h"

/**
 * @class FsVolumeManager manages the Volume at which the FileSystem
 * is stored on and provides and interface to access single sectors
 * (blocks), data-blocks (aka clusters) (compounds of singles sectors)
 * and keeps track of free and occupied sectors.
 */
class FsVolumeManager
{
public:
  /**
   * constructor
   *
   * @param file_system the associated FileSystem
   * @param device the Device that is managed by this FsVolumeManager instance
   * @param cache_factory the factory to produce the Device Sector Cache
   * @param cache_max_elements maximum number of sectors in the cache
   */
  FsVolumeManager(FileSystem* file_system, FsDevice* device,
                  Cache::CacheFactory* cache_factory,
                  Cache::num_items_t cache_max_elements);

  /**
   * destructor
   */
  virtual ~FsVolumeManager();

  /**
   * getting the Block Size of the FileSystem
   *
   * @return the block size in bytes
   */
  virtual sector_len_t getBlockSize(void) const;

  /**
   * reads out a Sector from the FileSystem's device and returns the read
   * data in a char array located on the heap. The Size of the array will
   * be exactly as long as the block-size of the FileSystem
   * (the current BlockSize can be obtained from getDataBlockSize())
   *
   * NOTE: This is an unsafe method and therefore it has to be called from
   * a safe (locked) context in multi-threading environments. (For details
   * look at acquireSectorForReading() or acquireSectorForWriting().
   *
   * WARNING: the Caller must NOT delete the obtained array! Always keep in
   * mind that this method just returns a pointer to the Sector data also
   * held in cache (shallow copy)!
   *
   * @param sector the number of the sector to be loaded
   * @return a char array holding the read sector's data
   */
  virtual char* readSectorUnprotected(sector_addr_t sector);

  /**
   * writes a Sector to the FileSystem's device
   * the size of the buffer has to be exactly as long as the block-size of
   * the FileSystem. If the buffer size does not exactly fit the method
   * will fail and might also lead to a general protection fault! BE CAREFULL!
   *
   * NOTE: This is an unsafe method and therefore it has to be called from
   * a safe (locked) write-context in multi-threading environments. (For details
   * look at acquireSectorForWriting().
   *
   * @param sector the sector number to be rewritten on the device
   */
  virtual bool writeSectorUnprotected(sector_addr_t sector, const char* buffer);

  /**
   * Synchronization methods for safe access to Device sectors
   * In Multithreading systems always call one of the two functions before
   * accessing a Sector's data in order to avoid inconsistency of data!
   *
   * An arbitrary number of sectors can be acquired for reading but at a
   * time only one can have write access to the Sector
   *
   * @param sector the Sector which shall be locked
   */
  virtual void acquireSectorForReading(sector_addr_t sector);
  virtual void acquireSectorForWriting(sector_addr_t sector);

  /**
   * after gaining exclusive safe access to a sector it has to be
   * released afterwards by calling one of the release methods!
   *
   * @param sector the Sector number to release
   * @param num_ref_releases the number of times the GeneralCache's releaseItem()
   * method shall be called (has to equal your number of performed read-operations)
   * by default 1 (assumes exactly one readSectorUnprotected() call in
   * the critical section)
   */
  virtual void releaseReadSector(sector_addr_t sector, uint32 num_ref_releases = 1);
  virtual void releaseWriteSector(sector_addr_t sector, uint32 num_ref_releases = 1);

  /**
   * getting the size of a *data* block on the FileSystem
   *
   * @return the *data* block size in bytes
   */
  virtual sector_len_t getDataBlockSize(void) const;

  /**
   * Synchronization methods for safe access to FileSystem's data blocks
   *
   * In Multithreading systems always call one of the two functions before
   * accessing a FileSystem's data block in order to avoid inconsistency of data!
   *
   * An arbitrary number of sectors can be acquired for reading but at a
   * time only one can have write access to the Sector
   *
   * @param data_block the data-block which shall be locked
   */
  virtual void acquireDataBlockForReading(sector_addr_t data_block);
  virtual void acquireDataBlockForWriting(sector_addr_t data_block);

  /**
   * after gaining exclusive safe access to a data-block it has to be
   * released afterwards by calling one of the release methods!
   *
   * @param data_block the data-block to release
   * @param num_ref_releases the number of times the GeneralCache's releaseItem()
   * method shall be called (has to equal your number of performed read-operations)
   * by default 1 (assumes exactly one readDataBlock() call in the critical
   * section)
   */
  virtual void releaseReadDataBlock(sector_addr_t data_block, uint32 num_ref_releases = 1);
  virtual void releaseWriteDataBlock(sector_addr_t data_block, uint32 num_ref_releases = 1);

  /**
   * reads out a data-block and returns it
   */
  virtual char* readDataBlockUnprotected(sector_addr_t sector);
  virtual bool writeDataBlockUnprotected(sector_addr_t sector, const char* block);

  /**
   * flushes all pending write-operations on the Device data block cache
   */
  void flush(void);

  /**
   * If there is a pending write-operation to the given sector it will be
   * executed right now.
   * If there is no pending write operation to the sector nothing will be
   * done at all.
   *
   * @param sector the data-block to write back right now
   * @return true if the block was found in the writing queue and successfully
   * written without delay. false in case of error
   */
  bool synchronizeSector(sector_addr_t sector);

  /**
   * getting cache-statistics
   * @param[out] stats
   */
  void getCacheStat(Cache::CacheStat& stats);

protected:

  // the associated FileSystem
  FileSystem* file_system_;

  // the managed Volume / Device
  FsDevice* device_;

  // the Locking Aid for the Sectors of the Volume
  SlotLockManager<sector_addr_t, uint32> sector_lock_manager_;

  // the Disk's sector cache
  Cache::GeneralCache* dev_sector_cache_;

private:

  void updateSectorData(sector_addr_t sector, const char* new_data, sector_len_t offset);

  /**
   * calls for the given sector num_release_calls-times the GeneralCache's
   * releaseItem() method
   *
   * @param sector
   * @param num_release_calls default
   */
  void decrRefCounterOfCacheElements(sector_addr_t sector, uint32 num_release_calls);
};

#endif /* FSVOLUMEMANAGER_H_ */
