/**
 * Filename: FifoReadCache.h
 * Description:
 *
 * Created on: 03.06.2012
 * Author: Christopher Walles
 */

#ifndef FIFOREADCACHE_H_
#define FIFOREADCACHE_H_

#ifdef USE_FILE_SYSTEM_ON_GUEST_OS
#include <vector>
#else
#include "uvector.h"
#endif

#include "GeneralCache.h"

namespace Cache
{

/**
 * @class a Read Cache Strategy realization
 */
class FifoReadCache : public CacheReadStrategy
{
public:
  FifoReadCache(GeneralCache* cache);
  virtual ~FifoReadCache();

  /**
   * searches the cache-data structure for an item with the
   * given identity and returns it's data if it was found
   * @param ident
   * @return data or NULL if not present
   */
  virtual Item* get(const ItemIdentity& ident);

  /**
   * adds a new item to the cache
   * @param ident the identity of the new item
   * @param item the item's data
   * @return true / false
   */
  virtual bool add(const ItemIdentity& ident, Item* item);

  /**
   * removes an item from the read-cache pool
   *
   * @param ident the Item with the given Identity will be removed
   */
  virtual void removeItem(const ItemIdentity& ident);

  /**
   * evicts an item according to the cache-strategy from the cache
   */
  virtual void evictItem(void);

  /**
   * clears all currently free items from the cache
   */
  virtual void clear(void);

  /**
   * returns the number of items in the cache
   * @return number of elements in the cache
   */
  virtual num_items_t getNumItems(void) const;

private:

  struct FIFOCacheItem
  {
    ItemIdentity* ident;
    Item* item;
  };

  // the FIFO approach can be modeled best by using a queue
  // the queue here is simulated by a simple vector
#ifdef USE_FILE_SYSTEM_ON_GUEST_OS
  typedef std::vector<FIFOCacheItem>::iterator fifo_it;
  std::vector<FIFOCacheItem> fifo_items_;
#else
  typedef ustl::vector<FIFOCacheItem>::iterator fifo_it;
  ustl::vector<FIFOCacheItem> fifo_items_;
  mutable Mutex fifo_mutex_;
#endif
};

} // end of namespace "Cache"

#endif /* FIFOREADCACHE_H_ */
