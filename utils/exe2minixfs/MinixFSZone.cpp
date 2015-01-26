 /**
 * @file MinixFSZone.cpp
 */

#include "MinixFSZone.h"
#include "MinixFSSuperblock.h"

#include <assert.h>
#include <iostream>

#include "minix_fs_consts.h"


MinixFSZone::MinixFSZone ( MinixFSSuperblock *superblock, zone_add_type *zones )
{
  superblock_ = superblock;
  direct_zones_ = new zone_add_type[9];
  Buffer *buffer = 0;
  num_zones_ = 0;
  for ( uint32 i = 0; i < 9; i++ )
  {
    direct_zones_[i] = zones[i];
    if ( zones[i] && i < 7 )
      ++num_zones_;
    //debug ( M_ZONE,"zone: %x\t", zones[i] );
  }
  if ( zones[7] )
  {
    indirect_zones_ = new zone_add_type[NUM_ZONE_ADDRESSES];
    buffer = new Buffer ( ZONE_SIZE );
    superblock_->readZone ( zones[7], buffer->getBuffer() );
    for ( uint32 i = 0; i < NUM_ZONE_ADDRESSES; i++ )
    {
      indirect_zones_[i] = buffer->get2Bytes ( i*2 );
      if ( buffer->get2Bytes ( i*2 ) )
        ++num_zones_;
    }
    delete buffer;
  }
  else
    indirect_zones_ = 0;
  if ( zones[8] )
  {
    double_indirect_zones_ = new zone_add_type*[NUM_ZONE_ADDRESSES];
    double_indirect_linking_zone_ = new zone_add_type[NUM_ZONE_ADDRESSES];
    buffer = new Buffer ( ZONE_SIZE );
    superblock_->readZone ( zones[8], buffer->getBuffer() );
    Buffer *ind_buffer = new Buffer ( ZONE_SIZE );
    for ( uint32 ind_zone = 0; ind_zone < NUM_ZONE_ADDRESSES; ind_zone++ )
    {
      double_indirect_linking_zone_[ind_zone] = buffer->get2Bytes ( ind_zone*2 );
      if ( buffer->get2Bytes ( ind_zone*2 ) )
      {
        superblock_->readZone ( buffer->get2Bytes ( ind_zone*2 ), ind_buffer->getBuffer() );
        double_indirect_zones_[ind_zone] = new zone_add_type[NUM_ZONE_ADDRESSES];
        for ( uint32 d_ind_zone = 0; d_ind_zone < NUM_ZONE_ADDRESSES; d_ind_zone++ )
        {
          double_indirect_zones_[ind_zone][d_ind_zone] = ind_buffer->get2Bytes ( d_ind_zone*2 );
          if ( ind_buffer->get2Bytes ( d_ind_zone*2 ) )
            ++num_zones_;
        }
      }
      else
        double_indirect_zones_[ind_zone] = 0;
    }
    delete buffer;
    delete ind_buffer;
  }
  else
  {
    double_indirect_zones_ = 0;
    double_indirect_linking_zone_ = 0;
  }

  //std::cout << this << "ctor: num-zones = " << num_zones_ << std::endl;

  /*if ( isDebugEnabled ( M_ZONE ) )
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
  }*/
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
  delete[] direct_zones_;
}


zone_add_type MinixFSZone::getZone ( uint32 index )
{
  //std::cout << "getZone: index: " << index << " zone of " << this << std::endl;
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
  //debug ( M_ZONE,"MinixFSZone::setZone> index: %d, zone: %d\n",index,zone );
  //std::cout << "setZone: index: " << index << " zone: " << zone << std::endl;
  if ( index < 7 )
  {
    direct_zones_[index] = zone;
    ++num_zones_;
    return;
  }
  index -= 7;
  if ( index < NUM_ZONE_ADDRESSES )
  {
    //std::cout << "setZone: indirect-index: " << index << " zone: " << zone << std::endl;
    if ( !indirect_zones_ )
    {
      direct_zones_[7] = superblock_->allocateZone();
      //std::cout << "setting up indirect-zone; direct_zones_[7] = " << direct_zones_[7] << std::endl;
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
  //std::cout << this << "flush: num-zones = " << num_zones_ << std::endl;
  //debug ( M_ZONE, "MinixFSZone::flush i_num : %d\n",i_num );
  Buffer *buffer = new Buffer ( 18 );
  for ( uint32 index = 0; index < 9; index++ )
  {
    buffer->set2Bytes ( index * 2, direct_zones_[index] );
  }
  uint32 block = 2 + superblock_->s_num_inode_bm_blocks_ + superblock_->s_num_zone_bm_blocks_ + ( ( i_num - 1 ) * INODE_SIZE ) / BLOCK_SIZE;
  superblock_->writeBytes ( block, ( ( i_num - 1 ) * INODE_SIZE ) % BLOCK_SIZE + 14, 18, buffer->getBuffer() );
  delete buffer;
  //debug ( M_ZONE,"MinixFSZone::flush direct written\n" );
  if ( direct_zones_[7] )
  {
    Buffer *ind_buffer = new Buffer ( ZONE_SIZE );
    //debug ( M_ZONE,"MinixFSZone::flush writing indirect\n" );
    assert ( indirect_zones_ );
    ind_buffer->clear();
    for ( uint32 i = 0; i < NUM_ZONE_ADDRESSES; i++ )
    {
      //if(indirect_zones_[i])
        //std::cout << "set2Bytes: indirect_zones_[i] = " << indirect_zones_[i] << std::endl;
      ind_buffer->set2Bytes ( i*2, indirect_zones_[i] );
    }
    //std::cout << "direct_zones_[7] = " << direct_zones_[7] << std::endl;
    superblock_->writeZone ( direct_zones_[7], ind_buffer->getBuffer() );
    delete ind_buffer;
  }

  if ( direct_zones_[8] )
  {
    Buffer *dbl_ind_buffer = new Buffer ( ZONE_SIZE );
    assert ( double_indirect_linking_zone_ );
    assert ( double_indirect_zones_ );
    for ( uint32 ind_zone = 0; ind_zone < NUM_ZONE_ADDRESSES; ind_zone++ )
    {
      dbl_ind_buffer->set2Bytes ( ind_zone*2, double_indirect_linking_zone_[ind_zone] );
    }
    superblock_->writeZone ( direct_zones_[8], dbl_ind_buffer->getBuffer() );
    for ( uint32 ind_zone = 0; ind_zone < NUM_ZONE_ADDRESSES; ind_zone++ )
    {
      if ( double_indirect_linking_zone_[ind_zone] )
      {
        dbl_ind_buffer->clear();
        for ( uint32 d_ind_zone = 0; d_ind_zone < NUM_ZONE_ADDRESSES; d_ind_zone++ )
        {
          dbl_ind_buffer->set2Bytes ( d_ind_zone*2, double_indirect_zones_[ind_zone][d_ind_zone] );
        }
        superblock_->writeZone ( double_indirect_linking_zone_[ind_zone], dbl_ind_buffer->getBuffer() );
      }
    }
    delete dbl_ind_buffer;
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

