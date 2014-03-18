/**
 * Filename: FifoReadWriteBackCacheFactory.cpp
 * Description:
 *
 * Created on: 04.09.2012
 * Author: chris
 */

#include "cache/FifoReadWriteBackCacheFactory.h"
#include "cache/GeneralCache.h"
#include "cache/FifoReadCache.h"
#include "cache/WriteBackCache.h"

namespace Cache
{

FifoReadWriteBackCacheFactory::FifoReadWriteBackCacheFactory()
{
  // TODO Auto-generated constructor stub

}

FifoReadWriteBackCacheFactory::~FifoReadWriteBackCacheFactory()
{
  // TODO Auto-generated destructor stub
}

CacheReadStrategy* FifoReadWriteBackCacheFactory::getReadStrategy(GeneralCache* cache) const
{
  return new FifoReadCache(cache);
}

CacheWriteStrategy* FifoReadWriteBackCacheFactory::getWriteStrategy(DeviceAdapter* device) const
{
  return new WriteBackCache(device);
}

} // end of namespace "cache"
