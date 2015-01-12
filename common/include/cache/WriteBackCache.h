/**
 * Filename: WriteBackCache.h
 * Description:
 *
 * Created on: 23.08.2012
 * Author: chris
 */

#ifndef WRITEBACKCACHE_H_
#define WRITEBACKCACHE_H_

#include "CacheStrategy.h"

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
#include "uvector.h"
#else
#include <vector>
#endif

namespace Cache
{

/**
 * @class A "write-back" strategy for the Cache
 */
class WriteBackCache : public CacheWriteStrategy
{
public:
  /**
   * constructor
   *
   * @param device
   */
  WriteBackCache(DeviceAdapter* device);

  /**
   * destructor
   */
  virtual ~WriteBackCache();

  /**
   * caches the used Cache::DeviceAdapter
   *
   * @param cache_device the new DeviceAdapter to use
   */
  virtual void setCacheDevice(DeviceAdapter* cache_device);

  /**
   * queues a new write operation for the given item
   * the CacheWriteStrategy implementation will decide when the Item
   * is written out to the Device
   * @param ident the item's identity
   * @param data the item's data to be written
   * @param delete_item
   * @return true if the write operation was successfully queued
   * false if the operation could not be queued
   */
  virtual bool queueWrite(const Cache::ItemIdentity& ident, Cache::Item* item, bool delete_item);

  /**
   * If the given item is in the write queue it will be taken out of it
   * and beeing immediately written to the Device without any delay!
   * If no write-operation is queued for the given Item nothing will be done!
   * This method is required for example in so called Write-Back strategies
   *
   * @param ident
   */
  virtual bool enforceWrite(const Cache::ItemIdentity& ident);

  /**
   * flushes all pending writing operations
   */
  virtual void flush(void);

  /**
   * removes an item from the write-cache pool
   * this method will discard all queued write operations of the item
   *
   * @param ident the Item with the given Identity will be removed
   */
  virtual void removeItem(const Cache::ItemIdentity& ident);

private:

  class WriteBackItem
  {
    public:
      WriteBackItem(Cache::ItemIdentity* id, Cache::Item* it_data, bool delete_itm);

      ~WriteBackItem();

      Cache::ItemIdentity* ident;
      Cache::Item* item;
      bool delete_item; // flag: delete item after writing
  };

  // use a vector as a waiting queue for write operations
#ifdef USE_FILE_SYSTEM_ON_GUEST_OS
  typedef std::vector<WriteBackItem*>::iterator queue_it;
  std::vector<WriteBackItem*> item_queue_;
#else
  typedef ustl::vector<WriteBackItem*>::iterator queue_it;
  ustl::vector<WriteBackItem*> item_queue_;
  mutable Mutex mutex_;
#endif
};

} // end of namespace

#endif /* WRITEBACKCACHE_H_ */
