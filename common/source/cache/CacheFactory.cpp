/**
 * Filename: CacheFactory.cpp
 * Description:
 *
 * Created on: 04.06.2012
 * Author: chris
 */

#include <assert.h>
#include "CacheFactory.h"

namespace Cache
{

CacheFactory::CacheFactory()
{
}

CacheFactory::~CacheFactory()
{
}

GeneralCache* CacheFactory::getNewCache(DeviceAdapter* device, num_items_t max_items) const
{
  GeneralCache* cache = new GeneralCache(device, 0, max_items);
  assert(cache != NULL);

  cache->setReadStrategy(getReadStrategy(cache));
  cache->setWriteStrategy(getWriteStrategy(device));

  return cache;
}

} // end of namespace "Cache"
