/**
 * @file KernelMemoryManager.cpp
 */

#include "mm/KernelMemoryManager.h"
#include "ArchCommon.h"
#include "assert.h"
#include "debug_bochs.h"
#include "console/kprintf.h"
#include "console/debug.h"

KernelMemoryManager * KernelMemoryManager::instance_ = 0;

uint32 KernelMemoryManager::createMemoryManager ( pointer start_address, pointer end_address )
{
  //start_address will propably be &kernel_end_address defined in linker script
  //since we don't have memory management before creating the MemoryManager, we use "placement new"
  instance_ = new ( ( void* ) start_address ) KernelMemoryManager ( start_address + sizeof ( KernelMemoryManager ),end_address );
  return 0;
}

KernelMemoryManager::KernelMemoryManager ( pointer start_address, pointer end_address )
{


  malloc_end_=end_address;
  prenew_assert ( ( ( end_address-start_address-sizeof ( MallocSegment ) ) & 0x80000000 ) == 0 );
  first_=new ( ( void* ) start_address ) MallocSegment ( 0,0,end_address-start_address-sizeof ( MallocSegment ),false );
  last_=first_;
  use_spinlock_=false;
  debug ( KMM,"KernelMemoryManager::ctor: bytes avaible: %d \n",end_address-start_address );
}

pointer KernelMemoryManager::allocateMemory ( size_t requested_size )
{
  lockKMM();
  // find next free pointer of neccessary size + sizeof(MallocSegment);
  prenew_assert ( ( requested_size & 0x80000000 ) == 0 );
  MallocSegment *new_pointer = findFreeSegment ( requested_size );

  if ( new_pointer == 0 )
  {
    unlockKMM();
    debug ( KMM,"KernelMemoryManager::allocateMemory: Not enough Memory left\n" );
    debug ( KMM,"KernelMemoryManager::allocateMemory: Not enough Memory left\n" );
    return 0;
  }

  fillSegment ( new_pointer,requested_size );

  unlockKMM();

  debug ( KMM,"allocateMemory returns address: %x \n", ( ( pointer ) new_pointer ) + sizeof ( MallocSegment ) );
  return ( ( pointer ) new_pointer ) + sizeof ( MallocSegment );

}

bool KernelMemoryManager::freeMemory ( pointer virtual_address )
{
  if ( virtual_address == 0 || virtual_address < ( ( pointer ) first_ ) || virtual_address >= malloc_end_ )
    return false;

  lockKMM();

  MallocSegment *m_segment = getSegmentFromAddress ( virtual_address );
  if ( m_segment->marker_ != 0xdeadbeef )
  {
    unlockKMM();
    return false;
  }
  freeSegment ( m_segment );

  unlockKMM();
  return true;
}

pointer KernelMemoryManager::reallocateMemory ( pointer virtual_address, size_t new_size )
{
  prenew_assert ( ( new_size & 0x80000000 ) == 0 );
  if ( new_size == 0 )
  {
    //in case you're wondering: we really don't want to lock here yet :) guess why
    freeMemory ( virtual_address );
    return 0;
  }

  lockKMM();

  MallocSegment *m_segment = getSegmentFromAddress ( virtual_address );

  if ( new_size == m_segment->getSize() )
  {
    unlockKMM();
    return virtual_address;
  }

  if ( new_size < m_segment->getSize() )
  {
    fillSegment ( m_segment,new_size );
    if ( m_segment->next_ != 0 )
    {
      prenew_assert ( m_segment->next_->marker_ == 0xdeadbeef );
      mergeWithFollowingFreeSegment ( m_segment->next_ );
    }
    unlockKMM();
    return virtual_address;
  }
  else
  {
    //maybe we can solve this the easy way...
    if ( m_segment->next_ != 0 )
      if ( m_segment->next_->getUsed() == false &&
              m_segment->next_->getSize() + m_segment->getSize() >= new_size )
      {
        mergeWithFollowingFreeSegment ( m_segment );
        unlockKMM();
        return virtual_address;
      }

    //or not.. lets search for larger space
    pointer new_address = allocateMemory ( new_size );
    ArchCommon::memcpy ( new_address,virtual_address, m_segment->getSize() );
    freeSegment ( m_segment );
    unlockKMM();
    return new_address;
  }
}


MallocSegment *KernelMemoryManager::getSegmentFromAddress ( pointer virtual_address )
{
  MallocSegment *m_segment;
  m_segment = ( MallocSegment* ) ( virtual_address - sizeof ( MallocSegment ) );
  prenew_assert ( m_segment != 0 );
  prenew_assert ( m_segment->marker_ == 0xdeadbeef );
  return m_segment;
}

MallocSegment *KernelMemoryManager::findFreeSegment ( size_t requested_size )
{
  debug ( KMM,"findFreeSegment: seeking memory block of bytes: %d \n",requested_size + sizeof ( MallocSegment ) );

  MallocSegment *current=first_;
  while ( current != 0 )
  {
    debug ( KMM,"findFreeSegment: current: %x size: %d used: %d \n", current, current->getSize() + sizeof ( MallocSegment ), current->getUsed() );
    prenew_assert ( current->marker_ == 0xdeadbeef );
    if ( ( current->getSize() >= requested_size ) && ( current->getUsed() == false ) )
      return current;

    current = current->next_;
  }

  return 0;
}

void KernelMemoryManager::fillSegment ( MallocSegment *this_one, size_t requested_size )
{
  prenew_assert ( this_one != 0 );
  prenew_assert ( this_one->marker_ == 0xdeadbeef );
  prenew_assert ( this_one->getSize() >= requested_size );

  size_t space_left = this_one->getSize() - requested_size;

  //size stays as it is, if there would be no more space to add a new segment
  this_one->setUsed ( true );
  prenew_assert ( this_one->getUsed() == true );

  //add a free segment after this one, if there's enough space
  if ( space_left > sizeof ( MallocSegment ) )
  {
    this_one->setSize ( requested_size );
    prenew_assert ( this_one->getSize() == requested_size );
    prenew_assert ( this_one->getUsed() == true );

    MallocSegment *new_segment = new ( ( void* ) ( ( ( pointer ) this_one ) +sizeof ( MallocSegment ) +requested_size ) ) MallocSegment ( this_one,this_one->next_,space_left-sizeof ( MallocSegment ),false );
    if ( this_one->next_ != 0 )
    {
      prenew_assert ( this_one->next_->marker_ == 0xdeadbeef );
      this_one->next_->prev_ = new_segment;
    }
    this_one->next_ = new_segment;

    if ( new_segment->next_ == 0 )
      last_ = new_segment;
  }
  debug ( KMM,"fillSegment: filled memory block of bytes: %d \n", this_one->getSize() + sizeof ( MallocSegment ) );
}

void KernelMemoryManager::freeSegment ( MallocSegment *this_one )
{
  prenew_assert ( this_one != 0 );
  prenew_assert ( this_one->marker_ == 0xdeadbeef );

  if ( this_one->getUsed() == false )
  {
    kprintfd ( "KernelMemoryManager::freeSegment: FATAL ERROR\n" );
    kprintfd ( "KernelMemoryManager::freeSegment: tried freeing not used memory block\n" );
    prenew_assert ( false );
  }

  debug ( KMM,"fillSegment: freeing block: %x of bytes: %d \n",this_one,this_one->getSize() + sizeof ( MallocSegment ) );

  this_one->setUsed ( false );
  prenew_assert ( this_one->getUsed() == false );

  if ( this_one->prev_ != 0 )
  {
    prenew_assert ( this_one->prev_->marker_ == 0xdeadbeef );
    if ( this_one->prev_->getUsed() == false )
    {
      size_t my_true_size = ( ( this_one->next_==0 ) ? malloc_end_ - ( ( pointer ) this_one ) : ( ( pointer ) this_one->next_ ) - ( ( pointer ) this_one ) );

      MallocSegment *previous_one = this_one->prev_;

      previous_one->setSize ( my_true_size + previous_one->getSize() );
      previous_one->next_ = this_one->next_;
      if ( this_one->next_ != 0 )
      {
        prenew_assert ( this_one->next_->marker_ == 0xdeadbeef );
        this_one->next_->prev_=previous_one;
      }

      debug ( KMM,"freeSegment: post premerge, pre postmerge\n" );
      debug ( KMM,"freeSegment: previous_one: %x size: %d used: %d\n",previous_one, previous_one->getSize() + sizeof ( MallocSegment ), previous_one->getUsed() );
      debug ( KMM,"freeSegment: this_one: %x size: %d used: %d\n",this_one, this_one->getSize() + sizeof ( MallocSegment ), this_one->getUsed() );

      this_one = previous_one;
    }
  }

  mergeWithFollowingFreeSegment ( this_one );

  //ease debugging, clear the segment we don't own anymore
  //and the previous MallocSegment Object with it.
  //this is really safe unless someone just grabs memory instead
  //of allocating it with this MemoryManager
  //is something is suddenly zero, we immediatly know there was a
  //problem with someones pointer
  ArchCommon::bzero ( ( ( pointer ) this_one ) + sizeof ( MallocSegment ), this_one->getSize(), 0 );

  //~ //debug code:
  if ( isDebugEnabled ( KMM ) )
  {
    MallocSegment *current=first_;
    while ( current != 0 )
    {
      debug ( KMM,"freeSegment: current: %x prev: %x next: %x size: %d used: %d\n",current, current->prev_, current->next_, current->getSize() + sizeof ( MallocSegment ), current->getUsed() );
      prenew_assert ( current->marker_ == 0xdeadbeef );
      current = current->next_;
    }

  }
}

bool KernelMemoryManager::mergeWithFollowingFreeSegment ( MallocSegment *this_one )
{
  prenew_assert ( this_one != 0 );
  prenew_assert ( this_one->marker_ == 0xdeadbeef );

  if ( this_one->next_ != 0 )
  {
    prenew_assert ( this_one->next_->marker_ == 0xdeadbeef );
    if ( this_one->next_->getUsed() == false )
    {
      MallocSegment *next_one = this_one->next_;
      size_t true_next_size = ( ( next_one->next_==0 ) ? malloc_end_ - ( ( pointer ) next_one ) : ( ( pointer ) next_one->next_ ) - ( ( pointer ) next_one ) );

      this_one->setSize ( this_one->getSize() + true_next_size );
      this_one->next_ = next_one->next_;
      if ( next_one->next_  != 0 )
      {
        prenew_assert ( next_one->next_->marker_ == 0xdeadbeef );
        next_one->next_->prev_=this_one;
      }

      ArchCommon::bzero ( ( pointer ) next_one, sizeof ( MallocSegment ), 0 );

      //have to check again, could have changed...
      if ( this_one->next_ == 0 )
        last_ = this_one;

      return true;
    }
  }
  return false;
}



//~ #ifndef isXenBuild
//~ void printout(char* text)
//~ {
//~ uint8 * fb = (uint8*)0xC00B8000;
//~ static uint32 i=160;

//~ while (text && *text)
//~ {
//~ fb[i++] = *text++;
//~ fb[i++] = 0x9f;
//~ }
//~ }
//~ #else
//~ #include "console/kprintf.h"
//~ void printout(char* text)
//~ {
//~ kprintf(text);
//~ }
//~ #endif


//~ static uint32 fb_start = 0;
//~ static char* fb = (char*)0xC00B8500;

//~ #ifndef isXenBuild
//~ #define print(x)     fb_start += 2;
//~ {
//~ uint32 divisor;
//~ uint32 current;
//~ uint32 remainder;
//~ current = (uint32)x;
//~ divisor = 1000000000;
//~ while (divisor > 0)
//~ {
//~ remainder = current % divisor;
//~ current = current / divisor;
//~
//~ fb[fb_start++] = (uint8)current + '0' ;
//~ fb[fb_start++] = 0x9f ;
//~
//~ divisor = divisor / 10;
//~ current = remainder;
//~ }
//~ uint32 blubba;
//~ uint32 asf;
//~ for (asf=0;asf<1;++asf)
//~ ++blubba;
//~ }
//~ #else
//~ #define print(x)
//~ #endif


//~ #define printbochs(x)
//~ {
//~ uint32 printbochs_divisor;
//~ uint32 printbochs_current;
//~ uint32 printbochs_remainder;
//~ printbochs_current = (uint32)x;
//~ printbochs_divisor = 1000000000;
//~ while (printbochs_divisor > 0)
//~ {
//~ printbochs_remainder = printbochs_current % printbochs_divisor;
//~ printbochs_current = printbochs_current / printbochs_divisor;
//~
//~ writeChar2Bochs('0' + (uint8)printbochs_current);
//~
//~ printbochs_divisor = printbochs_divisor / 10;
//~ printbochs_current = printbochs_remainder;
//~ }
//~ uint32 printbochs_blubba;
//~ uint32 printbochs_asf;
//~ for (printbochs_asf=0;printbochs_asf<1;++printbochs_asf)
//~ ++printbochs_blubba;
//~ }
