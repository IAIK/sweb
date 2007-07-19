//----------------------------------------------------------------------
//   $Id: KernelMemoryManager.cpp,v 1.22 2005/10/27 09:04:59 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: KernelMemoryManager.cpp,v $
//  Revision 1.21  2005/09/26 14:58:05  btittelbach
//  doxyfication
//
//  Revision 1.20  2005/09/13 15:00:51  btittelbach
//  Prepare to be Synchronised...
//  kprintf_nosleep works now
//  scheduler/list still needs to be fixed
//
//  Revision 1.19  2005/08/17 20:00:59  nomenquis
//  fuck the fucking fuckers
//
//  Revision 1.18  2005/08/11 18:28:10  nightcreature
//  changed define of evil print(x) depending on platform xen or x86
//
//  Revision 1.17  2005/08/07 16:47:25  btittelbach
//  More nice synchronisation Experiments..
//  RaceCondition/kprintf_nosleep related ?/infinite memory write loop Error still not found
//  kprintfd doesn't use a buffer anymore, as output_bochs blocks anyhow, should propably use some arch-specific interface instead
//
//  Revision 1.16  2005/07/12 19:58:40  btittelbach
//  minus debug garbage output
//
//  Revision 1.15  2005/07/12 19:52:25  btittelbach
//  Debugged evil evil double-linked-list bug
//
//  Revision 1.14  2005/07/12 17:52:26  btittelbach
//  Bochs SerialConsole ist jetzt lesbar
//  KMM hat rudimentäre Bochs Debug
//
//  Revision 1.13  2005/07/12 17:28:47  btittelbach
//  bochs kmm debug
//
//  Revision 1.12  2005/05/31 20:25:28  btittelbach
//  moved assert to where it belongs (arch) and created nicer version
//
//  Revision 1.11  2005/05/10 15:27:54  davrieb
//  move assert to util/assert.h
//
//  Revision 1.10  2005/05/03 18:31:10  btittelbach
//  fix of evil evil MemoryManager Bug
//
//  Revision 1.9  2005/05/03 17:09:25  btittelbach
//  Assert Fix
//
//  Revision 1.8  2005/04/28 14:07:25  btittelbach
//  Merge von size und flag (hack)
//
//  Revision 1.7  2005/04/26 15:58:46  nomenquis
//  threads, scheduler, happy day
//
//  Revision 1.6  2005/04/25 23:09:18  nomenquis
//  fubar 2
//
//  Revision 1.5  2005/04/25 21:15:41  nomenquis
//  lotsa changes
//
//  Revision 1.4  2005/04/23 22:20:30  btittelbach
//  just stuff
//
//  Revision 1.3  2005/04/23 18:13:27  nomenquis
//  added optimised memcpy and bzero
//  These still could be made way faster by using asm and using cache bypassing mov instructions
//
//  Revision 1.2  2005/04/23 17:35:03  nomenquis
//  fixed buggy memory manager
//  (giving out the same memory several times is no good idea)
//
//  Revision 1.1  2005/04/23 15:59:26  btittelbach
//
//  Testing Version vom KMM
//
//  Revision 1.2  2005/04/22 20:18:52  nomenquis
//  compile fixes
//
//  Revision 1.1  2005/04/22 20:13:00  btittelbach
//  Memory Manager v.0001
//
//
//----------------------------------------------------------------------
///
/// KernelMemoryManager
///
/// Management and Allocation of All that is Memory
///
/// @author Bernhard Tittelbach <xro@sbox.tugraz.at>
/// @version 1.0.0
/// 

#include "../../include/mm/KernelMemoryManager.h"
#include "ArchCommon.h"
#include "assert.h"
#include "../../../arch/arch/include/debug_bochs.h"
#include "console/kprintf.h"

KernelMemoryManager * KernelMemoryManager::instance_ = 0;

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
    
uint32 KernelMemoryManager::createMemoryManager(pointer start_address, pointer end_address)
{
  instance_ = new ((void*)start_address) KernelMemoryManager(start_address + sizeof(KernelMemoryManager),end_address);
  return 0;
}

KernelMemoryManager::KernelMemoryManager(pointer start_address, pointer end_address)
{ 
  

  malloc_end_=end_address;
  //memory_free_=end_address-start_address-sizeof(MallocSegment);
  prenew_assert (((end_address-start_address-sizeof(MallocSegment)) & 0x80000000) == 0);
  first_=new ((void*) start_address) MallocSegment(0,0,end_address-start_address-sizeof(MallocSegment),false);
  last_=first_;
  use_spinlock_=false;
  //segments_free_=1;
  //segments_used_=0;
  writeLine2Bochs((uint8*)"KernelMemoryManager::ctor: bytes avaible:");
  //printbochs(end_address-start_address);
  writeChar2Bochs((uint8)'\n');
  
}

pointer KernelMemoryManager::allocateMemory(size_t requested_size)
{
  lockKMM();
  // find next free pointer of neccessary size + sizeof(MallocSegment);
  prenew_assert ((requested_size & 0x80000000) == 0);
  MallocSegment *new_pointer = findFreeSegment(requested_size);
  
  if (new_pointer == 0)
  {
    unlockKMM();
    kprintfd_nosleep("KernelMemoryManager::allocateMemory: Not enough Memory left\n");
    kprintf_nosleep("KernelMemoryManager::allocateMemory: Not enough Memory left\n");
    return 0;
  }
  
  fillSegment(new_pointer,requested_size);

  unlockKMM();
  
  //print(((pointer) new_pointer) + sizeof(MallocSegment))
  //print(9877)
  return ((pointer) new_pointer) + sizeof(MallocSegment);

}
  
bool KernelMemoryManager::freeMemory(pointer virtual_address)
{
  //find MallocSegment
  //delete segment
  if (virtual_address == 0 || virtual_address < ((pointer) first_) || virtual_address >= malloc_end_)
    return false;
  
  lockKMM();
  
  MallocSegment *m_segment = getSegmentFromAddress(virtual_address);
  if (m_segment->marker_ != 0xdeadbeef)
  {
    unlockKMM();
    return false;
  }
  freeSegment(m_segment);
  
  unlockKMM();
  return true;
}
  
pointer KernelMemoryManager::reallocateMemory(pointer virtual_address, size_t new_size)
{
  //find MallocSegment
  //check if there is enought free space afterwards
  //if it is -> merge spaces and return same pointer
  //if not -> find large enough space and move memory, return new pointer
  prenew_assert ((new_size & 0x80000000) == 0);
  if (new_size == 0)
  {
    //in case you're wondering: we really don't want to lock here yet :) guess why
    freeMemory(virtual_address);
    return 0;
  }
  
  lockKMM();
  
  MallocSegment *m_segment = getSegmentFromAddress(virtual_address);
  
  if (new_size == m_segment->getSize())
  {
    unlockKMM();
    return virtual_address;
  }
  
  if (new_size < m_segment->getSize())
  { //downsize segment
    fillSegment(m_segment,new_size);
    if (m_segment->next_ != 0)
    {
      prenew_assert(m_segment->next_->marker_ == 0xdeadbeef);
      mergeWithFollowingFreeSegment(m_segment->next_);
    }
    unlockKMM();
    return virtual_address;
  }
  else
  { //maybe we can solve this the easy way...
    if (m_segment->next_ != 0)
      if (m_segment->next_->getUsed() == false &&
        m_segment->next_->getSize() + m_segment->getSize() >= new_size)
      {
        mergeWithFollowingFreeSegment(m_segment);
        unlockKMM();
        return virtual_address;
      }
      
    //or not.. lets search for larger space
    pointer new_address = allocateMemory(new_size);
    ArchCommon::memcpy(new_address,virtual_address, m_segment->getSize());
    freeSegment(m_segment);
    unlockKMM();
    return new_address;
  }
}


MallocSegment *KernelMemoryManager::getSegmentFromAddress(pointer virtual_address)
{
  MallocSegment *m_segment;
  m_segment = (MallocSegment*) (virtual_address - sizeof(MallocSegment));
  prenew_assert(m_segment != 0);
  prenew_assert(m_segment->marker_ == 0xdeadbeef);
  return m_segment;
}

MallocSegment *KernelMemoryManager::findFreeSegment(size_t requested_size)
{
  //~ writeLine2Bochs((uint8*)"KernelMemoryManager::findFreeSegment: seeking memory block of bytes:");
  //~ printbochs(requested_size + sizeof(MallocSegment));
  //~ writeChar2Bochs((uint8)'\n');

  
  MallocSegment *current=first_;
  while (current != 0)
  {
    //~ writeLine2Bochs((uint8*)"KernelMemoryManager::findFreeSegment: current:");
    //~ printbochs(current);
    //~ writeLine2Bochs((uint8*)" size:");
    //~ printbochs( current->getSize() + sizeof(MallocSegment));
    //~ writeLine2Bochs((uint8*)" used:");
    //~ printbochs( current->getUsed() );
    //~ writeChar2Bochs((uint8)'\n');
    prenew_assert(current->marker_ == 0xdeadbeef);
    if ((current->getSize() >= requested_size) && (current->getUsed() == false))
      return current;
    
    current = current->next_;
  }
  
  return 0;
}

void KernelMemoryManager::fillSegment(MallocSegment *this_one, size_t requested_size)
{
  prenew_assert(this_one != 0);
  prenew_assert(this_one->marker_ == 0xdeadbeef);
  prenew_assert(this_one->getSize() >= requested_size);

  size_t space_left = this_one->getSize() - requested_size;

  //size stays as it is, if there would be no more space to add a new segment
  this_one->setUsed(true);
  prenew_assert(this_one->getUsed() == true);

  //add a free segment after this one, it there's enough space
  if (space_left > sizeof(MallocSegment))
  {
    this_one->setSize(requested_size);
    prenew_assert(this_one->getSize() == requested_size);
    prenew_assert(this_one->getUsed() == true);
    
    MallocSegment *new_segment = new ((void*) ( ((pointer) this_one)+sizeof(MallocSegment)+requested_size)) MallocSegment(this_one,this_one->next_,space_left-sizeof(MallocSegment),false);
    if (this_one->next_ != 0)
    {
      prenew_assert(this_one->next_->marker_ == 0xdeadbeef);
      this_one->next_->prev_ = new_segment;
    }
    this_one->next_ = new_segment;
    
    if (new_segment->next_ == 0)
      last_ = new_segment;
  }
  //~ writeLine2Bochs((uint8*)"KernelMemoryManager::fillSegment: filled memory block of bytes:");
  //~ printbochs(this_one->getSize() + sizeof(MallocSegment));
  //~ writeChar2Bochs((uint8)'\n');
}

void KernelMemoryManager::freeSegment(MallocSegment *this_one)
{
  prenew_assert(this_one != 0);
  prenew_assert(this_one->marker_ == 0xdeadbeef);

  //mark segment as free
  //if previous segment is free: delete Segment and add space to previous Segmen
  //if next segment is free: delete next Segment and add space to this segment
  
  if (this_one->getUsed() == false) //that would be bad
  {
    writeLine2Bochs((uint8*)"KernelMemoryManager::freeSegment: FATAL ERROR\n");
    writeLine2Bochs((uint8*)"KernelMemoryManager::freeSegment: tried freeing not used memory block\n"); 
    prenew_assert(false);
  } 
  
  //~ writeLine2Bochs((uint8*)"KernelMemoryManager::fillSegment: freeing block ");
  //~ printbochs(this_one);   
  //~ writeLine2Bochs((uint8*)" of bytes:");
  //~ printbochs(this_one->getSize() + sizeof(MallocSegment));   
  //~ writeChar2Bochs((uint8)'\n');
  
  this_one->setUsed(false);
  prenew_assert(this_one->getUsed() == false);
  
  if (this_one->prev_ != 0)
  {
    prenew_assert(this_one->prev_->marker_ == 0xdeadbeef);
    if (this_one->prev_->getUsed() == false)
    {
      size_t my_true_size = ((this_one->next_==0)? malloc_end_ - ((pointer) this_one) : ((pointer) this_one->next_) - ((pointer) this_one));

      MallocSegment *previous_one = this_one->prev_;
      
      previous_one->setSize(my_true_size + previous_one->getSize());
      previous_one->next_ = this_one->next_;
      if (this_one->next_ != 0)
      {
        prenew_assert(this_one->next_->marker_ == 0xdeadbeef);
        this_one->next_->prev_=previous_one;
      }
      
      //~ writeLine2Bochs((uint8*)"KernelMemoryManager::freeSegment: post premerge, pre postmerge\n");
      //~ writeLine2Bochs((uint8*)"KernelMemoryManager::freeSegment: previous_one:");
      //~ printbochs(previous_one);
      //~ writeLine2Bochs((uint8*)" size:");
      //~ printbochs( previous_one->getSize() + sizeof(MallocSegment));
      //~ writeLine2Bochs((uint8*)" used:");
      //~ printbochs( previous_one->getUsed() );
      //~ writeChar2Bochs((uint8)'\n');
      //~ writeLine2Bochs((uint8*)"KernelMemoryManager::freeSegment: this_one:");
      //~ printbochs(this_one);
      //~ writeLine2Bochs((uint8*)" size:");
      //~ printbochs( this_one->getSize() + sizeof(MallocSegment));
      //~ writeLine2Bochs((uint8*)" used:");
      //~ printbochs( this_one->getUsed() );
      //~ writeChar2Bochs((uint8)'\n');
      
      this_one = previous_one;
    }
  }

  mergeWithFollowingFreeSegment(this_one);
   
  //ease debugging, clear the segment we don't own anymore
  //and the previous MallocSegment Object with it.
  //this is really safe unless someone just grabs memory instead
  //of allocating it with this MemoryManager
  //is something is suddenly zero, we immediatly know there was a 
  //problem with someones pointer
  ArchCommon::bzero(((pointer) this_one) + sizeof(MallocSegment), this_one->getSize(), 0);
  
  //~ //debug code:
  //~ MallocSegment *current=first_;
  //~ while (current != 0)
  //~ {
    //~ writeLine2Bochs((uint8*)"KernelMemoryManager::freeSegment: current:");
    //~ printbochs(current);
    //~ writeLine2Bochs((uint8*)" prev:");
    //~ printbochs( current->prev_);
    //~ writeLine2Bochs((uint8*)" next:");
    //~ printbochs( current->next_);
    //~ writeLine2Bochs((uint8*)" size:");
    //~ printbochs( current->getSize() + sizeof(MallocSegment));
    //~ writeLine2Bochs((uint8*)" used:");
    //~ printbochs( current->getUsed() );
    //~ writeChar2Bochs((uint8)'\n');
    //~ prenew_assert(current->marker_ == 0xdeadbeef);    
    //~ current = current->next_;
  //~ }

}

bool KernelMemoryManager::mergeWithFollowingFreeSegment(MallocSegment *this_one)
{
  prenew_assert(this_one != 0);
  prenew_assert(this_one->marker_ == 0xdeadbeef);
  
  if (this_one->next_ != 0)
  {
    prenew_assert(this_one->next_->marker_ == 0xdeadbeef);
    if (this_one->next_->getUsed() == false)
    {
      MallocSegment *next_one = this_one->next_;
      size_t true_next_size = ((next_one->next_==0)? malloc_end_ - ((pointer) next_one) : ((pointer) next_one->next_) - ((pointer) next_one));

      this_one->setSize(this_one->getSize() + true_next_size);
      this_one->next_ = next_one->next_;
      if (next_one->next_  != 0)
      {
        prenew_assert(next_one->next_->marker_ == 0xdeadbeef);
        next_one->next_->prev_=this_one;
      }

      ArchCommon::bzero((pointer) next_one, sizeof(MallocSegment), 0);
      
      //have to check again, could have changed...
      if (this_one->next_ == 0)
        last_ = this_one;
      
      return true;
    }
  }
  return false;
}
