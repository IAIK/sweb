/**
 * @file MinixFSZone.cpp
 */

#include "fs/minixfs/MinixFSZone.h"
#include "fs/minixfs/MinixFSSuperblock.h"

#include "console/kprintf.h"

#include "fs/minixfs/minix_fs_consts.h"
#include "ArchCommon.h"

MinixFSZone::MinixFSZone ( MinixFSSuperblock *superblock, zone_add_type *zones )
{
  superblock_ = superblock;
  num_zones_ = 0;
  for ( uint32 i = 0; i < 9; i++ )
  {
    direct_zones_[i] = zones[i];
    if ( zones[i] && i < 7 )
      ++num_zones_;
    debug ( M_ZONE,"zone: %x\t", zones[i] );
  }
  char buffer[ZONE_SIZE];
  uint16* temp = (uint16*) buffer;
  if ( zones[7] )
  {
    indirect_zones_ = new zone_add_type[NUM_ZONE_ADDRESSES];
    superblock_->readZone ( zones[7], buffer );
    for (uint32 i = 0; i < NUM_ZONE_ADDRESSES; i++)
    {
      indirect_zones_[i] = temp[i];
      if (indirect_zones_[i])
        ++num_zones_;
    }
  }
  else
  {
    indirect_zones_ = 0;
  }
  if ( zones[8] )
  {
    double_indirect_zones_ = new zone_add_type*[NUM_ZONE_ADDRESSES];
    double_indirect_linking_zone_ = new zone_add_type[NUM_ZONE_ADDRESSES];
    superblock_->readZone ( zones[8], buffer );
    char ind_buffer[ZONE_SIZE];
    uint16* ind_temp = (uint16*) ind_buffer;
    for ( uint32 ind_zone = 0; ind_zone < NUM_ZONE_ADDRESSES; ind_zone++ )
    {
      double_indirect_linking_zone_[ind_zone] = temp[ind_zone];
      if ( double_indirect_linking_zone_[ind_zone] )
      {
        superblock_->readZone ( double_indirect_linking_zone_[ind_zone], ind_buffer );
        double_indirect_zones_[ind_zone] = new zone_add_type[NUM_ZONE_ADDRESSES];

        for ( uint32 d_ind_zone = 0; d_ind_zone < NUM_ZONE_ADDRESSES; d_ind_zone++ )
        {
          double_indirect_zones_[ind_zone][d_ind_zone] = ind_temp[d_ind_zone];
          if (double_indirect_zones_[ind_zone][d_ind_zone])
            ++num_zones_;
        }
      }
      else
      {
        double_indirect_zones_[ind_zone] = 0;
      }
    }
  }
  else
  {
    double_indirect_zones_ = 0;
    double_indirect_linking_zone_ = 0;
  }

  if ( isDebugEnabled ( M_ZONE ) )
  {
    kprintfd ( "=========Zones:======%d=======\n",num_zones_ );
    kprintfd ( "====direct Zones:====\n" );
    uint32 print_num_zones = 0;
    for ( uint32 i = 0; i < 9; i++,print_num_zones++ )
    {
      kprintfd ( "====zone: %x\t", direct_zones_[i] );
    }
    kprintfd ( "===indirect Zones:===\n" );
    for ( uint32 i = 0; i < NUM_ZONE_ADDRESSES && print_num_zones < num_zones_; i++,print_num_zones++ )
    {
      kprintfd ( "===zone: %x\t", indirect_zones_[i] );
    }
    kprintfd ( "=dblindirect Zones:==\n" );
    for ( uint32 ind_zone = 0; ind_zone < NUM_ZONE_ADDRESSES && print_num_zones < num_zones_; ind_zone++,print_num_zones++ )
    {
      for ( uint32 d_ind_zone = 0; d_ind_zone < NUM_ZONE_ADDRESSES && print_num_zones < num_zones_; d_ind_zone++,print_num_zones++ )
      {
        kprintfd ( "=zone: %x\t", double_indirect_zones_[ind_zone][d_ind_zone] );
      }
    }
  }
}


MinixFSZone::~MinixFSZone()
{
  if ( double_indirect_zones_ )
  {
    for ( uint32 i = 0; i < NUM_ZONE_ADDRESSES; i++ )
    {
      delete[] double_indirect_zones_[i];
    }
    delete[] double_indirect_zones_;
    delete[] double_indirect_linking_zone_;
  }

  delete[] indirect_zones_;
}


zone_add_type MinixFSZone::getZone ( uint32 index )
{
  assert ( index < num_zones_ );
  if ( index < 7 )
  {
    return direct_zones_[index];
  }
  index -= 7;
  if ( index < NUM_ZONE_ADDRESSES )
  {
    return indirect_zones_[index];
  }
  index -= NUM_ZONE_ADDRESSES;
  return double_indirect_zones_[index/NUM_ZONE_ADDRESSES][index%NUM_ZONE_ADDRESSES];
}


void MinixFSZone::setZone ( uint32 index, zone_add_type zone )
{
  debug ( M_ZONE,"MinixFSZone::setZone> index: %d, zone: %d\n",index,zone );
  if ( index < 7 )
  {
    direct_zones_[index] = zone;
    ++num_zones_;
    return;
  }
  index -= 7;
  if ( index < NUM_ZONE_ADDRESSES )
  {
    if ( !indirect_zones_ )
    {
      direct_zones_[7] = superblock_->allocateZone();
      indirect_zones_ = new zone_add_type[NUM_ZONE_ADDRESSES];
      for(uint32 i=0; i < NUM_ZONE_ADDRESSES; i++)
        indirect_zones_[i] = 0;
    }
    indirect_zones_[index] = zone;
    ++num_zones_;
    return;
  }
  index -= NUM_ZONE_ADDRESSES;
  if ( !double_indirect_zones_ )
  {
    direct_zones_[8] = superblock_->allocateZone();
    double_indirect_linking_zone_ = new zone_add_type[NUM_ZONE_ADDRESSES];
    for(uint32 i=0; i < NUM_ZONE_ADDRESSES; i++)
      double_indirect_linking_zone_[i] = 0;

    double_indirect_zones_ = new zone_add_type*[NUM_ZONE_ADDRESSES];
    for(uint32 i=0; i < NUM_ZONE_ADDRESSES; i++)
      double_indirect_zones_[i] = 0;
  }
  if ( !double_indirect_zones_[index/NUM_ZONE_ADDRESSES] )
  {
    double_indirect_linking_zone_[index/NUM_ZONE_ADDRESSES] = superblock_->allocateZone();
    double_indirect_zones_[index/NUM_ZONE_ADDRESSES] = new zone_add_type[NUM_ZONE_ADDRESSES];
    for(uint32 i=0; i < NUM_ZONE_ADDRESSES; i++)
      double_indirect_zones_[index/NUM_ZONE_ADDRESSES][i] = 0;
  }
  double_indirect_zones_[index/NUM_ZONE_ADDRESSES][index%NUM_ZONE_ADDRESSES] = zone;

  ++num_zones_;
}


void MinixFSZone::addZone ( zone_add_type zone )
{
  setZone ( num_zones_, zone );
}


void MinixFSZone::flush ( uint32 i_num )
{
  debug ( M_ZONE, "MinixFSZone::flush i_num : %d; %x\n",i_num, this );
  char buffer[18];
  for ( uint32 index = 0; index < 9; index++ )
  {
    *(uint16*)(buffer + index * 2) = direct_zones_[index];
  }
  uint32 block = 2 + superblock_->s_num_inode_bm_blocks_ + superblock_->s_num_zone_bm_blocks_ + ( ( i_num - 1 ) * INODE_SIZE ) / BLOCK_SIZE;
  superblock_->writeBytes ( block, ( ( i_num - 1 ) * INODE_SIZE ) % BLOCK_SIZE + 14, 18, buffer );
  debug ( M_ZONE,"MinixFSZone::flush direct written\n" );
  if ( direct_zones_[7] )
  {
    char ind_buffer[ZONE_SIZE];
    debug ( M_ZONE,"MinixFSZone::flush writing indirect\n" );
    assert ( indirect_zones_ );
    ArchCommon::bzero((pointer)ind_buffer,sizeof(ind_buffer));
    for ( uint32 i = 0; i < NUM_ZONE_ADDRESSES; i++ )
    {
      *(uint16*)(ind_buffer + i*2) = indirect_zones_[i];
    }
    superblock_->writeZone ( direct_zones_[7], ind_buffer );
  }

  if ( direct_zones_[8] )
  {
    char dbl_ind_buffer[ZONE_SIZE];
    assert ( double_indirect_linking_zone_ );
    assert ( double_indirect_zones_ );
    for ( uint32 ind_zone = 0; ind_zone < NUM_ZONE_ADDRESSES; ind_zone++ )
    {
      *(uint16*)(dbl_ind_buffer + ind_zone*2) = double_indirect_linking_zone_[ind_zone];
    }
    superblock_->writeZone ( direct_zones_[8], dbl_ind_buffer );
    for ( uint32 ind_zone = 0; ind_zone < NUM_ZONE_ADDRESSES; ind_zone++ )
    {
      if ( double_indirect_linking_zone_[ind_zone] )
      {
        ArchCommon::bzero((pointer)dbl_ind_buffer,sizeof(dbl_ind_buffer));
        for ( uint32 d_ind_zone = 0; d_ind_zone < NUM_ZONE_ADDRESSES; d_ind_zone++ )
        {
          *(uint16*)(dbl_ind_buffer + d_ind_zone*2) = double_indirect_zones_[ind_zone][d_ind_zone];
        }
        superblock_->writeZone ( double_indirect_linking_zone_[ind_zone], dbl_ind_buffer );
      }
    }
  }
}

void MinixFSZone::freeZones()
{
 for(uint32 i=0; i < 9; i++)
   if(direct_zones_[i])
     superblock_->freeZone(direct_zones_[i]);

 if(!indirect_zones_)
   return;

 for(uint32 i=0; i < NUM_ZONE_ADDRESSES; i++)
   if(indirect_zones_[i])
     superblock_->freeZone(indirect_zones_[i]);

 if(!double_indirect_linking_zone_)
   return;

 for(uint32 i=0; i < NUM_ZONE_ADDRESSES; i++)
   if(double_indirect_linking_zone_[i])
     superblock_->freeZone(double_indirect_linking_zone_[i]);

 if(!double_indirect_zones_)
   return;

 for(uint32 i=0; i < NUM_ZONE_ADDRESSES; i++)
   if(double_indirect_zones_[i])
     for(uint32 j=0; j < NUM_ZONE_ADDRESSES; j++)
       if(double_indirect_zones_[i][j])
         superblock_->freeZone(double_indirect_zones_[i][j]);
}

