/**
 * Filename: FiFoReadNoWriteCacheFactory.cpp
 * Description:
 *
 * Created on: 04.06.2012
 * Author: chris
 */

#include "FiFoReadNoWriteCacheFactory.h"
#include "../cache/FifoReadCache.h"

namespace Cache
{

FiFoReadNoWriteCacheFactory::FiFoReadNoWriteCacheFactory()
{
}

FiFoReadNoWriteCacheFactory::~FiFoReadNoWriteCacheFactory()
{
}

CacheReadStrategy* FiFoReadNoWriteCacheFactory::getReadStrategy(GeneralCache* cache) const
{
  return new FifoReadCache(cache);
}

CacheWriteStrategy* FiFoReadNoWriteCacheFactory::getWriteStrategy(DeviceAdapter* device __attribute__((unused))) const
{
  // no writing cache! -> WriteThrough
  return NULL;
}

} // end of namespace "Cache"
