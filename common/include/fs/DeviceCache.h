/**
 * Filename: DeviceCache.h
 * Description: implementes the neccesary classes to use the GeneralCache
 * like CacheItems and their Identity object to compare and identify them
 * in the Cache
 *
 * Created on: 06.06.2012
 * Author: chris
 */

#ifndef DEVICECACHE_H_INCLUDED_
#define DEVICECACHE_H_INCLUDED_

#include "cache/CacheItem.h"
#include "FsDefinitions.h"

/**
 * @class the Cache's identity object for the Block-device data
 */
class SectorCacheIdent : public Cache::ItemIdentity
{
public:
  /**
   * construtor
   *
   * @param sector_no
   * @param block_size
   */
  SectorCacheIdent(sector_addr_t sector_no, sector_len_t block_size);

  virtual ~SectorCacheIdent();

  /**
   * Comparison operator for two CacheItems
   */
  virtual bool operator==(const Cache::ItemIdentity& cmp);

  /**
   * clones this object and returns a new instance of it
   * Located on the heap
   * @return an identical copy of this identity
   */
  virtual Cache::ItemIdentity* clone(void) const;

  /**
   * getting the sector number
   */
  sector_addr_t getSectorNumber(void) const;

  /**
   * getting the used block-size
   *
   * @return the used block size
   */
  sector_len_t getSectorSize(void) const;

private:

  // the sector-number is the ID
  sector_addr_t sector_no_;

  // the used block-size:
  sector_len_t block_size_;
};

/**
 * @class the SectorCache Item
 */
class SectorCacheItem : public Cache::Item
{
public:
  SectorCacheItem(char* sector_data);
  virtual~ SectorCacheItem();

  virtual void* getData(void);
  virtual const void* getData(void) const;

private:

  char* sector_;
};

#endif /* DEVICECACHE_H_INCLUDED_ */
