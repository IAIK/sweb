/**
 * Filename: FifoReadCache.cpp
 * Description:
 *
 * Created on: 03.06.2012
 * Author: Christopher Walles
 */

#include "FifoReadCache.h"

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
#include "assert.h"
#include "kprintf.h"
#else
#include <assert.h>
#include "debug_print.h"
#endif

namespace Cache
{

FifoReadCache::FifoReadCache(GeneralCache* cache) : CacheReadStrategy(cache)
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
    , fifo_mutex_("FifoReadCache Mutex")
#endif
{
}

FifoReadCache::~FifoReadCache()
{
  clear();
  assert(fifo_items_.size() == 0); // assertion indicates a locking fault!
}

Item* FifoReadCache::get(const ItemIdentity& ident)
{
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  MutexLock auto_lock(fifo_mutex_);
#endif

  // search the queue for an element with the given id
  for(num_items_t i = 0; i < fifo_items_.size(); i++)
  {
    if(*fifo_items_[i].ident == ident)
      return fifo_items_[i].item; // Cache-Hit!
  }

  return NULL;
}

bool FifoReadCache::add(const ItemIdentity& ident, Item* item)
{
  if(item == NULL)
    return false;

  FIFOCacheItem new_cache_item;
  new_cache_item.ident = ident.clone();
  new_cache_item.item = item;

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  MutexLock auto_lock(fifo_mutex_);
#endif

  fifo_items_.push_back(new_cache_item);
  return true;
}

void FifoReadCache::removeItem(const ItemIdentity& ident)
{
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  MutexLock auto_lock(fifo_mutex_);
#endif

  for(num_items_t i = 0; i < fifo_items_.size(); i++)
  {
    if(*fifo_items_[i].ident == ident)
    {
      delete fifo_items_[i].item;
      delete fifo_items_[i].ident;

      // remove item from pool
      fifo_items_.erase(fifo_items_.begin()+i);
      break;
    }
  }
}

void FifoReadCache::evictItem(void)
{
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  MutexLock auto_lock(fifo_mutex_);
#endif

  // search for the first free item
  for(num_items_t i = 0; i < fifo_items_.size(); i++)
  {
    if(cache_->isItemFree(*fifo_items_[i].ident))
    {
      // before deleting, write the item back
      cache_->writeItemImmediately(*fifo_items_[i].ident);

      delete fifo_items_[i].item;
      delete fifo_items_[i].ident;
      fifo_items_.erase(fifo_items_.begin()+i);
      return;
    }
  }

  debug(READ_CACHE, "evictItem - num_items=%d\n", fifo_items_.size());

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  debug(READ_CACHE, "evictItem - FAIL could not delete an item!\n");
#endif
}

void FifoReadCache::clear(void)
{
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  MutexLock auto_lock(fifo_mutex_);
#endif

  for(fifo_it it = fifo_items_.begin(); it != fifo_items_.end(); /*NO INCREMENT HERE!!!*/)
  {
    if(cache_->isItemFree(*(*it).ident))
    {
      delete (*it).item;
      delete (*it).ident;
      fifo_items_.erase(it);
    }
    else
    {
      it++; // pick next element
      debug(READ_CACHE, "evictItem - FAIL could not clear item because it is still referenced!\n");
    }
  }
}

num_items_t FifoReadCache::getNumItems(void) const
{
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  MutexLock auto_lock(fifo_mutex_);
#endif

  return fifo_items_.size();
}

} // end of namespace "Cache"
