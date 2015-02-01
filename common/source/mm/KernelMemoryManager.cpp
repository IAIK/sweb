#include "KernelMemoryManager.h"
#include "ArchCommon.h"
#include "assert.h"
#include "debug_bochs.h"
#include "kprintf.h"
#include "debug.h"
#include "Scheduler.h"
#include "ArchInterrupts.h"
#include "kstring.h"

extern uint32 boot_completed;

KernelMemoryManager * KernelMemoryManager::instance_ = 0;

KernelMemoryManager* KernelMemoryManager::instance()
{
  if (unlikely(!instance_))
  {
    pointer start_address = ArchCommon::getFreeKernelMemoryStart();
    pointer end_address = ArchCommon::getFreeKernelMemoryEnd();
    //start_address will propably be &kernel_end_address defined in linker script
    //since we don't have memory management before creating the MemoryManager, we use "placement new"
    instance_ = new ((void*) start_address) KernelMemoryManager(start_address + sizeof(KernelMemoryManager), end_address);
  }
  return instance_;
}

KernelMemoryManager::KernelMemoryManager(pointer start_address, pointer end_address) :
    lock_("KMM::lock_"), segments_used_(0), segments_free_(0), approx_memory_free_(0)
{
  malloc_end_ = end_address;
  prenew_assert(((end_address - start_address - sizeof(MallocSegment)) & 0xFFFFFFFF80000000) == 0);
  first_ = new ((void*) start_address) MallocSegment(0, 0, end_address - start_address - sizeof(MallocSegment), false);
  last_ = first_;
  debug(KMM, "KernelMemoryManager::ctor: bytes avaible: %d \n", end_address - start_address);
  debug(KMM, "ArchCommon::bzero((pointer) %x, %x,1);\n", (pointer) first_ + sizeof(MallocSegment),
        end_address - start_address - sizeof(MallocSegment));
  memset((void*) ((size_t) first_ + sizeof(MallocSegment)), 0, end_address - start_address - sizeof(MallocSegment));
}

pointer KernelMemoryManager::allocateMemory(size_t requested_size)
{
  prenew_assert((requested_size & 0x80000000) == 0);
  if ((requested_size & 0xF) != 0)
    requested_size += 0x10 - (requested_size & 0xF); // 16 byte alignment
  lockKMM();
  pointer ptr = private_AllocateMemory(requested_size);
  if (ptr)
    unlockKMM();

  debug(KMM, "allocateMemory returns address: %x \n", ptr);
  return ptr;
}
pointer KernelMemoryManager::private_AllocateMemory(size_t requested_size)
{
  // find next free pointer of neccessary size + sizeof(MallocSegment);
  MallocSegment *new_pointer = findFreeSegment(requested_size);

  if (new_pointer == 0)
  {
    unlockKMM();
    kprintfd("KernelMemoryManager::allocateMemory: Not enough Memory left\n");
    kprintfd("Are we having a memory leak in the kernel??\n");
    kprintfd(
        "This might as well be caused by running too many threads/processes, which partially reside in the kernel.\n");
    assert(false);
    return 0;
  }

  fillSegment(new_pointer, requested_size);
  return ((pointer) new_pointer) + sizeof(MallocSegment);
}

bool KernelMemoryManager::freeMemory(pointer virtual_address)
{
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
  prenew_assert((new_size & 0x80000000) == 0);
  if (new_size == 0)
  {
    //in case you're wondering: we really don't want to lock here yet :) guess why
    freeMemory(virtual_address);
    return 0;
  }
  //iff the old segment is no segment ;) -> we create a new one
  if (virtual_address == 0)
    return allocateMemory(new_size);

  lockKMM();

  MallocSegment *m_segment = getSegmentFromAddress(virtual_address);

  if (new_size == m_segment->getSize())
  {
    unlockKMM();
    return virtual_address;
  }

  if (new_size < m_segment->getSize())
  {
    fillSegment(m_segment, new_size, 0);
    unlockKMM();
    return virtual_address;
  }
  else
  {
    //maybe we can solve this the easy way...
    if (m_segment->next_ != 0)
      if (m_segment->next_->getUsed() == false && m_segment->next_->getSize() + m_segment->getSize() >= new_size)
      {
        mergeWithFollowingFreeSegment(m_segment);
        fillSegment(m_segment, new_size, 0);
        unlockKMM();
        return virtual_address;
      }

    //or not.. lets search for larger space

    //thx to Philipp Toeglhofer we are not going to deadlock here anymore ;)
    pointer new_address = private_AllocateMemory(new_size);
    if (new_address == 0)
    {
      //we are not freeing the old semgent in here, so that the data is not
      //getting lost, although we could not allocate more memory 

      //just if you wonder: the KMM is already unlocked
      kprintfd("KernelMemoryManager::reallocateMemory: Not enough Memory left\n");
      kprintfd("Are we having a memory leak in the kernel??\n");
      kprintfd(
          "This might as well be caused by running too many threads/processes, which partially reside in the kernel.\n");
      assert(false);
      return 0;
    }
    memcpy((void*) new_address, (void*) virtual_address, m_segment->getSize());
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
  debug(KMM, "findFreeSegment: seeking memory block of bytes: %d \n", requested_size + sizeof(MallocSegment));

  MallocSegment *current = first_;
  while (current != 0)
  {
    debug(KMM, "findFreeSegment: current: %x size: %d used: %d \n", current, current->getSize() + sizeof(MallocSegment),
          current->getUsed());
    prenew_assert(current->marker_ == 0xdeadbeef);
    if ((current->getSize() >= requested_size) && (current->getUsed() == false))
      return current;

    current = current->next_;
  }

  return 0;
}

void KernelMemoryManager::fillSegment(MallocSegment *this_one, size_t requested_size, uint32 zero_check)
{
  prenew_assert(this_one != 0);
  prenew_assert(this_one->marker_ == 0xdeadbeef);
  prenew_assert(this_one->getSize() >= requested_size);
  uint32* mem = (uint32*) (this_one + 1);
  if (zero_check)
  {
    uint32 checked = 0;
    for (uint32 i = 0; i < requested_size / 4; ++i)
    {
      if (checked)
      {
        //kprintfd("%x\n", mem[i]);
      }
      else if (mem[i] != 0)
      {
        checked = 1;
        kprintfd("KernelMemoryManager::fillSegment: WARNING: Memory not zero at %x:\n", mem + i);
      }
    }
  }

  size_t space_left = this_one->getSize() - requested_size;

  //size stays as it is, if there would be no more space to add a new segment
  this_one->setUsed(true);
  prenew_assert(this_one->getUsed() == true);

  //add a free segment after this one, if there's enough space
  if (space_left > sizeof(MallocSegment))
  {
    this_one->setSize(requested_size);
    prenew_assert(this_one->getSize() == requested_size);
    prenew_assert(this_one->getUsed() == true);

    MallocSegment *new_segment =
        new ((void*) (((pointer) this_one) + sizeof(MallocSegment) + requested_size)) MallocSegment(
            this_one, this_one->next_, space_left - sizeof(MallocSegment), false);
    if (this_one->next_ != 0)
    {
      prenew_assert(this_one->next_->marker_ == 0xdeadbeef);
      this_one->next_->prev_ = new_segment;
    }
    this_one->next_ = new_segment;

    if (new_segment->next_ == 0)
      last_ = new_segment;
  }
  debug(KMM, "fillSegment: filled memory block of bytes: %d \n", this_one->getSize() + sizeof(MallocSegment));
}

void KernelMemoryManager::freeSegment(MallocSegment *this_one)
{
  prenew_assert(this_one != 0);
  prenew_assert(this_one->marker_ == 0xdeadbeef);

  if (this_one->getUsed() == false)
  {
    kprintfd("KernelMemoryManager::freeSegment: FATAL ERROR\n");
    kprintfd("KernelMemoryManager::freeSegment: tried freeing not used memory block\n");
    prenew_assert(false);
  }

  debug(KMM, "fillSegment: freeing block: %x of bytes: %d \n", this_one, this_one->getSize() + sizeof(MallocSegment));

  this_one->setUsed(false);
  prenew_assert(this_one->getUsed() == false);

  if (this_one->prev_ != 0)
  {
    prenew_assert(this_one->prev_->marker_ == 0xdeadbeef);
    if (this_one->prev_->getUsed() == false)
    {
      size_t my_true_size = (
          (this_one->next_ == 0) ? malloc_end_ - ((pointer) this_one) :
                                   ((pointer) this_one->next_) - ((pointer) this_one));

      MallocSegment *previous_one = this_one->prev_;

      previous_one->setSize(my_true_size + previous_one->getSize());
      previous_one->next_ = this_one->next_;
      if (this_one->next_ != 0)
      {
        prenew_assert(this_one->next_->marker_ == 0xdeadbeef);
        this_one->next_->prev_ = previous_one;
      }

      debug(KMM, "freeSegment: post premerge, pre postmerge\n");
      debug(KMM, "freeSegment: previous_one: %x size: %d used: %d\n", previous_one,
            previous_one->getSize() + sizeof(MallocSegment), previous_one->getUsed());
      debug(KMM, "freeSegment: this_one: %x size: %d used: %d\n", this_one, this_one->getSize() + sizeof(MallocSegment),
            this_one->getUsed());

      this_one = previous_one;
    }
  }

  mergeWithFollowingFreeSegment(this_one);

  memset((void*) ((size_t) this_one + sizeof(MallocSegment)), 0, this_one->getSize()); // ease debugging

  if (isDebugEnabled(KMM))
  {
    MallocSegment *current = first_;
    while (current != 0)
    {
      debug(KMM, "freeSegment: current: %x prev: %x next: %x size: %d used: %d\n", current, current->prev_,
            current->next_, current->getSize() + sizeof(MallocSegment), current->getUsed());
      prenew_assert(current->marker_ == 0xdeadbeef);
      current = current->next_;
    }

  }
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
      size_t true_next_size = (
          (next_one->next_ == 0) ? malloc_end_ - ((pointer) next_one) :
                                   ((pointer) next_one->next_) - ((pointer) next_one));

      this_one->setSize(this_one->getSize() + true_next_size);
      this_one->next_ = next_one->next_;
      if (next_one->next_ != 0)
      {
        prenew_assert(next_one->next_->marker_ == 0xdeadbeef);
        next_one->next_->prev_ = this_one;
      }

      memset((void*) next_one, 0, sizeof(MallocSegment));

      //have to check again, could have changed...
      if (this_one->next_ == 0)
        last_ = this_one;

      return true;
    }
  }
  return false;
}

Thread* KernelMemoryManager::KMMLockHeldBy()
{
  return lock_.heldBy();
}

void KernelMemoryManager::lockKMM()
{
  lock_.acquire();
}

void KernelMemoryManager::unlockKMM()
{
  lock_.release();
}

SpinLock& KernelMemoryManager::getKMMLock()
{
  return lock_;
}
