#include "fs/minixfs/MinixFSZone.h"
#include "fs/minixfs/MinixFSSuperblock.h"

#include "console/kprintf.h"

#include "fs/minixfs/minix_fs_consts.h"


MinixFSZone::MinixFSZone(MinixFSSuperblock *superblock, zone_add_type *zones)
{
  superblock_ = superblock;
  direct_zones_ = new zone_add_type[9];
  Buffer *buffer = 0;
  num_zones_ = 0;
  for(uint32 i = 0; i < 9; i++)
  {
    direct_zones_[i] = zones[i];
    if(zones[i])
      ++num_zones_;
//     kprintfd("zone: %x\t", zones[i]);
  }
//   kprintfd("\n");
  if(zones[7])
  {
    indirect_zones_ = new zone_add_type[NUM_ZONE_ADDRESSES];
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
    double_indirect_zones_ = new zone_add_type*[NUM_ZONE_ADDRESSES];
    double_indirect_linking_zone_ = new zone_add_type[NUM_ZONE_ADDRESSES];
    buffer = new Buffer(ZONE_SIZE);
    superblock_->readZone( zones[8], buffer);
    Buffer *ind_buffer = new Buffer(ZONE_SIZE);
    for(uint32 ind_zone = 0; ind_zone < NUM_ZONE_ADDRESSES; ind_zone++)
    {
      double_indirect_linking_zone_[ind_zone] = buffer->get2Bytes( ind_zone*2 );
      if(buffer->get2Bytes( ind_zone*2 ))
      {
        superblock_->readZone( buffer->get2Bytes( ind_zone*2 ), ind_buffer);
        double_indirect_zones_[ind_zone] = new zone_add_type[NUM_ZONE_ADDRESSES];
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

zone_add_type MinixFSZone::getZone(uint32 index)
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

void MinixFSZone::setZone(uint32 index, zone_add_type zone)
{
  kprintfd("MinixFSZone::setZone> index: %d, zone: %d\n",index,zone);
  if (index < 7)
  {
    direct_zones_[index] = zone;
    ++num_zones_;
    return;
  }
  index -= 7;
  if (index < NUM_ZONE_ADDRESSES)
  {
    if(!indirect_zones_)
    {
      direct_zones_[7] = superblock_->allocateZone();
      indirect_zones_ = new zone_add_type[NUM_ZONE_ADDRESSES];
    }
    indirect_zones_[index] = zone;
    ++num_zones_;
    return;
  }
  index -= NUM_ZONE_ADDRESSES;
  if(!double_indirect_zones_)
  {
    direct_zones_[8] = superblock_->allocateZone();
    double_indirect_linking_zone_ = new zone_add_type[NUM_ZONE_ADDRESSES];
    double_indirect_zones_ = new zone_add_type*[NUM_ZONE_ADDRESSES];
  }
  if(!double_indirect_zones_[index/NUM_ZONE_ADDRESSES])
  {
    double_indirect_linking_zone_[index/NUM_ZONE_ADDRESSES] = superblock_->allocateZone();
    double_indirect_zones_[index/NUM_ZONE_ADDRESSES] = new zone_add_type[NUM_ZONE_ADDRESSES];
  }
  double_indirect_zones_[index/NUM_ZONE_ADDRESSES][index%NUM_ZONE_ADDRESSES] = zone;
  
  ++num_zones_;
}

void MinixFSZone::addZone(zone_add_type zone)
{
  setZone(num_zones_, zone);
}

void MinixFSZone::flush(uint32 i_num)
{
  Buffer* buffer = new Buffer(18);
  for (uint32 index = 0; index < 9; index++ )
  {
    buffer->set2Bytes( index * 2, direct_zones_[index]);
  }
  uint32 block = 2 + superblock_->s_num_inode_bm_blocks_ + superblock_->s_num_zone_bm_blocks_ + (i_num * INODE_SIZE) / BLOCK_SIZE;
  superblock_->writeBytes( block, (i_num * INODE_SIZE) % BLOCK_SIZE + 14, 18, buffer);

  if(direct_zones_[7])
  {
    assert(indirect_zones_);
    buffer->clear();
    for(uint32 i = 0; i < NUM_ZONE_ADDRESSES; i++)
    {
      buffer->set2Bytes( i*2, indirect_zones_[i]);
    }
    superblock_->writeZone(direct_zones_[7], buffer);
  }

  if(direct_zones_[8])
  {
    assert(double_indirect_linking_zone_);
    assert(double_indirect_zones_);
    for(uint32 ind_zone = 0; ind_zone < NUM_ZONE_ADDRESSES; ind_zone++)
    {
      buffer->set2Bytes( ind_zone*2, double_indirect_linking_zone_[ind_zone] );
    }
    superblock_->writeZone( direct_zones_[8], buffer );
    for(uint32 ind_zone = 0; ind_zone < NUM_ZONE_ADDRESSES; ind_zone++)
    {
      if(double_indirect_linking_zone_[ind_zone])
      {
        buffer->clear();
        for(uint32 d_ind_zone = 0; d_ind_zone < NUM_ZONE_ADDRESSES; d_ind_zone++)
        {
          buffer->set2Bytes(d_ind_zone*2, double_indirect_zones_[ind_zone][d_ind_zone]);
        }
        superblock_->writeZone( double_indirect_linking_zone_[ind_zone], buffer);
      }
    }
  }
}
