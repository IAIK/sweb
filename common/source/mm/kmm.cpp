//----------------------------------------------------------------------
//   $Id: kmm.cpp,v 1.1 2005/04/22 20:13:00 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: kmm.cpp,v $
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

#include "../../include/mm/kmm.h"

KernelMemoryManager::KernelMemoryManager(pointer start_address, pointer end_address)
{
  if (start_address == 0)
    start_address=((pointer) this)+sizeof(this);
  
  assert (start_address < end_address);

  //just to make sure:  
  memoryZero(start_address,end_address-start_address);

  malloc_end_=end_address;
  memory_free_=end_address-start_address-sizeof(MallocSegment);
  first_=new ((void*) start_address) MallocSegment(0,0,memory_free_,false);
  last_=first_;
  segments_free_=1;
  segments_used_=0;
}

pointer KernelMemoryManager::allocateMemory(size_t requested_size)
{
  // find next free pointer of neccessary size + sizeof(MallocSegment);
  MallocSegment *new_pointer = findFreeSegment(requested_size);
  
  if (new_pointer == 0)
    arch_panic("Not enough Memory left\n");
  
  fillSegment(new_pointer,requested_size);

  return ((pointer) new_pointer) + sizeof(MallocSegment);

}
  
bool KernelMemoryManager::freeMemory(pointer virtual_address)
{
  //find MallocSegment
  //delete segment
  if (virtual_address == 0 || virtual_address < ((pointer) first_) || virtual_address >= malloc_end_)
    return false;
  MallocSegment *m_segment = getSegmentFromAddress(virtual_address);
  if (m_segment->marker_ != 0xdeadbeef)
    return false;
  freeSegment(m_segment);
  return true;
}
  
pointer KernelMemoryManager::reallocateMemory(pointer virtual_address, size_t new_size)
{
  //find MallocSegment
  //check if there is enought free space afterwards
  //if it is -> merge spaces and return same pointer
  //if not -> find large enough space and move memory, return new pointer
  if (new_size == 0)
  {
    freeMemory(virtual_address);
    return 0;
  }
  
  MallocSegment *m_segment = getSegmentFromAddress(virtual_address);
  
  if (new_size == m_segment->size_)
    return virtual_address;
  
  if (new_size < m_segment->size_)
  { //downsize segment
    downsizeSegment(m_segment,new_size);
    return virtual_address;
  }
  else
  { //maybe we can solve this the easy way...
    if (m_segment->next_ != 0)
      if (m_segment->next_->flag_ == 0 &&
        m_segment->next_->size_ + m_segment->size_ >= new_size)
      {
        mergeWithFollowingFreeSegment(m_segment);
        return virtual_address;
      }
      
    //or not.. lets search for larger space
    pointer new_address = allocateMemory(new_size);
    memoryCopy(virtual_address, new_address, m_segment->size_);
    freeSegment(m_segment);
    return new_address;
  }
}




//---Private Helper Functions--------------------------

//this could be much faster
void KernelMemoryManager::memoryCopy(pointer source, pointer destination, size_t size)
{
  for (uint8 *addr=0; addr < (uint8*)size; ++addr)
    *(destination+addr)=*(source+addr);  
}

//this could propably be optimized as well
void KernelMemoryManager::memoryZero(pointer virtual_address, size_t size)
{
  for (uint8 *addr=(uint8*)virtual_address; addr < (uint8*)(virtual_address + size); ++addr)
    *addr='\0';

}


MallocSegment *KernelMemoryManager::getSegmentFromAddress(pointer virtual_address)
{
  MallocSegment *m_segment;
  m_segment = (MallocSegment*) (virtual_address - sizeof(MallocSegment));
  assert(m_segment != 0);
  assert(m_segment->marker_ == 0xdeadbeef);
  return m_segment;
}

MallocSegment *KernelMemoryManager::findFreeSegment(size_t requested_size)
{
  MallocSegment *current=first_;
  while (current != 0)
  {
    assert(current->marker_ == 0xdeadbeef);
    if (current->size_ >= requested_size)
      return current;
    
    current = current->next_;
  }
  
  return 0;
}

void KernelMemoryManager::fillSegment(MallocSegment *this_one, size_t requested_size)
{
  assert(this_one != 0);
  assert(this_one->marker_ == 0xdeadbeef);
  assert(this_one->size_ >= requested_size);

  size_t space_left = this_one->size_ - requested_size;

  this_one->flag_=1;

  //add a free segment after this one, it there's enough space
  if (space_left > sizeof(MallocSegment))
  {
    this_one->size_=requested_size;
    
    MallocSegment *new_segment = new ((void*) ( ((pointer) this_one)+sizeof(MallocSegment)+requested_size)) MallocSegment(this_one,this_one->next_,space_left-sizeof(MallocSegment),false);
    this_one->next_ = new_segment;
  }
}

void KernelMemoryManager::freeSegment(MallocSegment *this_one)
{
  assert(this_one != 0);
  assert(this_one->marker_ == 0xdeadbeef);

  //mark segment as free
  //if previous segment is free: delete Segment and add space to previous Segmen
  //if next segment is free: delete next Segment and add space to this segment
   
  this_one->flag_ = 0;
  
  if (this_one->prev_ != 0)
  {
    assert(this_one->prev_->marker_ == 0xdeadbeef);
    if (this_one->prev_->flag_ == 0)
    {
      size_t my_true_size = ((this_one->next_==0)? malloc_end_ - ((pointer) this_one) : ((pointer) this_one->next_) - ((pointer) this_one));
      assert (my_true_size <= this_one->size_ + (2*sizeof(MallocSegment)));

      //this_one->prev_->size_ += this_one->size_ + sizeof(MallocSegment);
      this_one->prev_->size_ += my_true_size;
      this_one->prev_->next_ = this_one->next_;
      
      MallocSegment *previous_one = this_one->prev_;
      
      this_one->marker_=0;
      this_one->next_=0;
      this_one->size_=0;
      this_one->prev_=0;
      
      this_one = previous_one;
    }
  }
  
  mergeWithFollowingFreeSegment(this_one);
   
  //make it all nice and clean and debug friendly
  memoryZero( ((pointer) this_one)+sizeof(MallocSegment), this_one->size_);
}

bool KernelMemoryManager::mergeWithFollowingFreeSegment(MallocSegment *this_one)
{
  assert(this_one != 0);
  assert(this_one->marker_ == 0xdeadbeef);
  
  if (this_one->next_ != 0)
  {
    assert(this_one->next_->marker_ == 0xdeadbeef);
    if (this_one->next_->flag_ == 0)
    {
      MallocSegment *next_one = this_one->next_;
      size_t true_next_size = ((next_one->next_==0)? malloc_end_ - ((pointer) next_one) : ((pointer) next_one->next_) - ((pointer) next_one));
      assert (true_next_size <= next_one->size_ + (2*sizeof(MallocSegment)));
      //this_one->size_ += next_one->size_ + sizeof(MallocSegment);
      this_one->size_ += true_next_size;
      this_one->next_ = next_one->next_;
      
      next_one->marker_=0;
      next_one->next_=0;
      next_one->size_=0;
      next_one->prev_=0;

      //have to check again, could have changed...
      if (this_one->next_ == 0)
        last_ = this_one;
      
      return true;
    }
  }
  return false;
}

void KernelMemoryManager::downsizeSegment(MallocSegment *this_one, size_t new_size)
{
  assert(this_one != 0);
  assert(this_one->marker_ == 0xdeadbeef);
  assert(this_one->size_ >= new_size);

  size_t space_free = this_one->size_ - new_size;

  //add a free segment after this one, it there's enough space
  if (space_free > sizeof(MallocSegment))
  {
    this_one->size_=new_size;
    
    MallocSegment *new_segment = new ((void*) ( ((pointer) this_one)+sizeof(MallocSegment)+new_size)) MallocSegment(this_one,this_one->next_,space_free-sizeof(MallocSegment),false);
    this_one->next_ = new_segment;
    //check if we can enlarge...
    mergeWithFollowingFreeSegment(new_segment);
  }
}
