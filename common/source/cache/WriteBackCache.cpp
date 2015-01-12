/**
 * Filename: WriteBackCache.cpp
 * Description:
 *
 * Created on: 23.08.2012
 * Author: chris
 */

#include "cache/GeneralCache.h"
#include "cache/WriteBackCache.h"
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
#include "kprintf.h"
#else
#include "debug_print.h"
#endif

namespace Cache
{

WriteBackCache::WriteBackCache(DeviceAdapter* device) : CacheWriteStrategy(device)
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
, mutex_("WriteBackCache")
#endif
{
}

WriteBackCache::~WriteBackCache()
{
  flush();
}

void WriteBackCache::setCacheDevice(DeviceAdapter* cache_device)
{
  device_ = cache_device;
}

bool WriteBackCache::queueWrite(const Cache::ItemIdentity& ident, Cache::Item* item, bool delete_item)
{
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  mutex_.acquire("WriteBackCache::queueWrite");
#endif

  // check if there is already write operation for the given Ident ...
  for(uint32 i = 0; i < item_queue_.size(); i++)
  {
    if(*(item_queue_[i]->ident) == ident)
    {
      // remove the old item, because a new one, will follow now
      delete item_queue_[i];
      item_queue_.erase( item_queue_.begin() + i );

      debug(WRITE_CACHE, "queueWrite - already an item with that ident, remove it\n");
      break;
    }
  }

  // now insert the new item to the writing queue
  item_queue_.push_back( new WriteBackItem(ident.clone(), item, delete_item) );

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  mutex_.release("WriteBackCache::queueWrite");
#endif

  return true;
}

bool WriteBackCache::enforceWrite(const Cache::ItemIdentity& ident)
{
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  MutexLock auto_lock(mutex_);
#endif

  for(uint32 i = 0; i < item_queue_.size(); i++)
  {
    if(*(item_queue_[i]->ident) == ident)
    {
      debug(WRITE_CACHE, "enforceWrite - writing back %d (item=%x)\n", i, item_queue_[i]->item);

      // writing item back to the Device and remove it from the list
      device_->write(*item_queue_[i]->ident, item_queue_[i]->item);

      delete item_queue_[i];
      item_queue_.erase(item_queue_.begin() + i);
      return true;
    }
  }

  // error item was not found, sorry
  return false;
}

void WriteBackCache::flush(void)
{
  debug(WRITE_CACHE, "flush - CALL\n");

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  MutexLock auto_lock(mutex_);
#endif

  debug(WRITE_CACHE, "flush - %d pending items\n", item_queue_.size());

  for(uint32 i = 0; i < item_queue_.size(); i++)
  {
    debug(WRITE_CACHE, "flush - writing back %d (item=%x)\n", i, item_queue_[i]->item);

    // writing item back to the Device and remove it from the list
    device_->write(*item_queue_[i]->ident, item_queue_[i]->item);
    delete item_queue_[i];
  }

  item_queue_.clear();
}

void WriteBackCache::removeItem(const Cache::ItemIdentity& ident)
{
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  MutexLock auto_lock(mutex_);
#endif

  for(uint32 i = 0; i < item_queue_.size(); i++)
  {
    if(*(item_queue_[i]->ident) == ident)
    {
      delete item_queue_[i];
      item_queue_.erase(item_queue_.begin() + i);

      return;
    }
  }
}

WriteBackCache::WriteBackItem::WriteBackItem(Cache::ItemIdentity* id, Cache::Item* it_data, bool delete_itm) :
    ident(id), item(it_data), delete_item(delete_itm)
{
}

WriteBackCache::WriteBackItem::~WriteBackItem()
{
  debug(WRITE_CACHE, "~WriteBackItem CALL\n");

  if(delete_item) delete item;
  delete ident;
}

} // end of namespace

