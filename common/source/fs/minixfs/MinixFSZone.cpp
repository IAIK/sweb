#include "fs/minixfs/MinixFSZone.h"
#include "fs/minixfs/MinixFSSuperblock.h"

#include "console/kprintf.h"


MinixFSZone::MinixFSZone(MinixFSSuperblock *superblock, uint16 *zones)
{
  superblock_ = superblock;
  direct_zones_ = new uint16[9];
  Buffer *buffer = 0;
  num_zones_ = 0;
  for(uint32 i = 0; i < 9; i++)
  {
    direct_zones_[i] = zones[i];
    if(zones[i])
      ++num_zones_;
    kprintfd("zone: %x\t", zones[i]);
  }
  kprintfd("\n");
  if(zones[7])
  {
    indirect_zones_ = new uint16[NUM_ZONE_ADDRESSES];
    buffer = new Buffer(ZONE_SIZE);
    superblock_->readZone( zones[7], buffer);
    for(uint32 i = 0; i < NUM_ZONE_ADDRESSES; i++)
    {
      indirect_zones_[i] = buffer->get2Bytes( i*2 );
      if(buffer->get2Bytes( i*2 ))
        ++num_zones_;
    }
    delete buffer;
  }
  else
    indirect_zones_ = 0;
  if(zones[8])
  {
    double_indirect_zones_ = new uint16*[NUM_ZONE_ADDRESSES];
    double_indirect_linking_zone_ = new uint16[NUM_ZONE_ADDRESSES];
    buffer = new Buffer(ZONE_SIZE);
    superblock_->readZone( zones[8], buffer);
    Buffer *ind_buffer = new Buffer(ZONE_SIZE);
    for(uint32 ind_zone = 0; ind_zone < NUM_ZONE_ADDRESSES; ind_zone++)
    {
      double_indirect_linking_zone_[ind_zone] = buffer->get2Bytes( ind_zone*2 );
      if(buffer->get2Bytes( ind_zone*2 ))
      {
        superblock_->readZone( buffer->get2Bytes( ind_zone*2 ), ind_buffer);
        double_indirect_zones_[ind_zone] = new uint16[NUM_ZONE_ADDRESSES];
        for(uint32 d_ind_zone = 0; d_ind_zone < NUM_ZONE_ADDRESSES; d_ind_zone++)
        {
          double_indirect_zones_[ind_zone][d_ind_zone] = ind_buffer->get2Bytes( d_ind_zone*2 );
          if(ind_buffer->get2Bytes( d_ind_zone*2 ))
            ++num_zones_;
        }
      }
    }
    delete buffer;
    delete ind_buffer;
  }
  else
    double_indirect_zones_ = 0;
}




MinixFSZone::~MinixFSZone()
{
  if(double_indirect_zones_)
  {
    for(uint32 i = 0; i < NUM_ZONE_ADDRESSES; i++)
    {
      delete[] double_indirect_zones_[i];
    }
    delete[] double_indirect_zones_;
  }
  if(indirect_zones_)
  {
    delete[] indirect_zones_;
  }
  delete[] direct_zones_;
}

uint16 MinixFSZone::getZone(uint32 index)
{
  assert(index < num_zones_);
  if (index < 7)
  {
    return direct_zones_[index];
  }
  index -= 7;
  if (index < NUM_ZONE_ADDRESSES)
  {
    return indirect_zones_[index];
  }
  index -= NUM_ZONE_ADDRESSES;
  return double_indirect_zones_[index/NUM_ZONE_ADDRESSES][index%NUM_ZONE_ADDRESSES];
}

void MinixFSZone::setZone(uint32 index, uint16 zone)
{
  if (index < 7)
  {
    direct_zones_[index] = zone;
  }
  index -= 7;
  if (index < NUM_ZONE_ADDRESSES)
  {
    if(!indirect_zones_)
    {
      direct_zones_[7] = superblock_->allocateZone();
      indirect_zones_ = new uint16[NUM_ZONE_ADDRESSES];
    }
    indirect_zones_[index] = zone;
  }
  index -= NUM_ZONE_ADDRESSES;
  if(!double_indirect_zones_)
  {
    direct_zones_[8] = superblock_->allocateZone();
    double_indirect_linking_zone_ = new uint16[NUM_ZONE_ADDRESSES];
    double_indirect_zones_ = new uint16*[NUM_ZONE_ADDRESSES];
  }
  if(!double_indirect_zones_[index/NUM_ZONE_ADDRESSES])
  {
    double_indirect_linking_zone_[index/NUM_ZONE_ADDRESSES] = superblock_->allocateZone();
    double_indirect_zones_[index/NUM_ZONE_ADDRESSES] = new uint16[NUM_ZONE_ADDRESSES];
  }
  double_indirect_zones_[index/NUM_ZONE_ADDRESSES][index%NUM_ZONE_ADDRESSES] = zone;
  
  ++num_zones_;
}

void MinixFSZone::addZone(uint16 zone)
{
  setZone(num_zones_, zone);
}
