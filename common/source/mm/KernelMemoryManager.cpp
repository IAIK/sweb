#include "KernelMemoryManager.h"
#include "ArchCommon.h"
#include "assert.h"
#include "debug_bochs.h"
#include "kprintf.h"
#include "debug.h"
#include "Scheduler.h"
#include "ArchInterrupts.h"
#include "ArchMemory.h"
#include "PageManager.h"
#include "kstring.h"

KernelMemoryManager kmm;

KernelMemoryManager * KernelMemoryManager::instance_;
size_t KernelMemoryManager::pm_ready_;

KernelMemoryManager* KernelMemoryManager::instance()
{
  if (unlikely(!instance_))
  {
    assert(false && "you can not use KernelMemoryManager::instance before the PageManager is ready!");
  }
  return instance_;
}

KernelMemoryManager::KernelMemoryManager(size_t min_heap_pages, size_t max_heap_pages) :
    lock_("KMM::lock_"), segments_used_(0), segments_free_(0), approx_memory_free_(0)
{
  assert(instance_ == 0);
  instance_ = this;
  pointer start_address = ArchCommon::getFreeKernelMemoryStart();
  prenew_assert(((start_address) % PAGE_SIZE) == 0);
  base_break_ = start_address;
  kernel_break_ = start_address + min_heap_pages * PAGE_SIZE;
  reserved_min_ = min_heap_pages * PAGE_SIZE;
  reserved_max_ = max_heap_pages * PAGE_SIZE;
  debug(KMM, "Clearing initial heap pages\n");
  memset((void*)start_address, 0, min_heap_pages * PAGE_SIZE);
  first_ = (MallocSegment*)start_address;
  new ((void*)start_address) MallocSegment(0, 0, min_heap_pages * PAGE_SIZE - sizeof(MallocSegment), false);
  last_ = first_;
  debug(KMM, "KernelMemoryManager::ctor, Heap starts at %x and initially ends at %x\n", start_address, start_address + min_heap_pages * PAGE_SIZE);
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
  if (virtual_address == 0 || virtual_address < ((pointer) first_) || virtual_address >= kernel_break_)
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
      prenew_assert(false);
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
  // No free segment found, could we allocate more memory?
  if(last_->getUsed())
  {
    // In this case we have to create a new segment...
    MallocSegment* new_segment = new ((void*)ksbrk(sizeof(MallocSegment) + requested_size)) MallocSegment(last_, 0, requested_size, 0);
    last_->next_ = new_segment;
    last_ = new_segment;
  }
  else
  {
    // else we just increase the size of the last segment
    size_t needed_size = requested_size - last_->getSize();
    ksbrk(needed_size);
    last_->setSize(requested_size);
  }

  return last_;
}

void KernelMemoryManager::fillSegment(MallocSegment *this_one, size_t requested_size, uint32 zero_check)
{
  prenew_assert(this_one != 0);
  prenew_assert(this_one->marker_ == 0xdeadbeef);
  prenew_assert(this_one->getSize() >= requested_size);
  uint32* mem = (uint32*) (this_one + 1);
  if (zero_check)
  {
    for (uint32 i = 0; i < requested_size / 4; ++i)
    {
      if(unlikely(mem[i] != 0))
      {
        kprintfd("KernelMemoryManager::fillSegment: WARNING: Memory not zero at %x\n", mem + i);
        mem[i] = 0;
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
  debug(KMM, "KernelMemoryManager::freeSegment(%x)\n", this_one);
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
          (this_one->next_ == 0) ? kernel_break_ - ((pointer) this_one) :
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

  // Change break if this is the last segment
  if(this_one == last_)
  {
    if(this_one != first_)
    {
      // Default case, there are three sub cases
      // 1. we can free the whole segment because it is above the reserved minimum
      // 2. we can not touch the segment because it is below the reserved minimum
      // 3. we can shrink the size of the segment because a part of it is above the reserved minimum
      if((size_t)this_one > base_break_ + reserved_min_)
      {
        // Case 1
        prenew_assert(this_one && this_one->prev_ && this_one->prev_->marker_ == 0xdeadbeef);
        this_one->prev_->next_ = 0;
        last_ = this_one->prev_;
        ksbrk(-(this_one->getSize() + sizeof(MallocSegment)));
      }
      else if((size_t)this_one + sizeof(MallocSegment) + this_one->getSize() <= base_break_ + reserved_min_)
      {
        // Case 2
        // This is easy, just relax and do nothing
      }
      else
      {
        // Case 3
        // First calculate the new size of the segment
        size_t segment_size = (base_break_ + reserved_min_) - ((size_t)this_one + sizeof(MallocSegment));
        // Calculate how much we have to sbrk
        ssize_t sub = segment_size - this_one->getSize();
        ksbrk(sub);
        this_one->setSize(segment_size);
      }
    }
    else
    {
      if((this_one->getSize() - reserved_min_))
      {
        ksbrk(-(this_one->getSize() - reserved_min_));
        this_one->setSize(reserved_min_);
      }
    }
  }

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
          (next_one->next_ == 0) ? kernel_break_ - ((pointer) next_one) :
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

pointer KernelMemoryManager::ksbrk(ssize_t size)
{
  debug(KMM, "KernelMemoryManager::ksbrk(%d)\n", size);
  prenew_assert(base_break_ <= (size_t)kernel_break_ + size);
  prenew_assert(reserved_max_ == 0 || ((kernel_break_ - base_break_) + size) <= reserved_max_);
  debug(KMM, "KernelMemoryManager::ksbrk(%d)\n", size);
  if(size != 0)
  {
    debug(KMM, "KernelMemoryManager::ksbrk(%d)0\n", size);
    size_t old_brk = kernel_break_;
    size_t cur_top_vpn = kernel_break_ / PAGE_SIZE;
    if ((kernel_break_ % PAGE_SIZE) == 0)
      cur_top_vpn--;
    kernel_break_ = ((size_t)kernel_break_) + size;
    size_t new_top_vpn = (kernel_break_ )  / PAGE_SIZE;
    if ((kernel_break_ % PAGE_SIZE) == 0)
      new_top_vpn--;
    debug(KMM, "KernelMemoryManager::ksbrk(%d)1\n", size);
    if(size > 0)
    {
      debug(KMM, "%x != %x\n", cur_top_vpn, new_top_vpn);
      while(cur_top_vpn != new_top_vpn)
      {
        debug(KMM, "%x != %x\n", cur_top_vpn, new_top_vpn);
        cur_top_vpn++;
        assert(pm_ready_);
        size_t new_page = PageManager::instance()->allocPPN();
        if(unlikely(new_page == 0))
        {
          debug(KMM, "KernelMemoryManager::ksbrk(%d)4\n", size);
          kprintfd("KernelMemoryManager::freeSegment: FATAL ERROR\n");
          kprintfd("KernelMemoryManager::freeSegment: no more physical memory\n");
          prenew_assert(new_page != 0);
        }
        debug(KMM, "kbsrk: map %x -> %x\n", cur_top_vpn, new_page);
        memset((void*)ArchMemory::getIdentAddressOfPPN(new_page), 0 , PAGE_SIZE);
        ArchMemory::mapKernelPage(cur_top_vpn, new_page);
      }

    }
    else
    {
      debug(KMM, "KernelMemoryManager::ksbrk(%d)7\n", size);
      while(cur_top_vpn != new_top_vpn)
      {
        prenew_assert(pm_ready_);
        ArchMemory::unmapKernelPage(cur_top_vpn);
        cur_top_vpn--;
      }
    }
    debug(KMM, "KernelMemoryManager::ksbrk(%d)8\n", size);
    return old_brk;
  }
  else
  {
    debug(KMM, "KernelMemoryManager::ksbrk(%d)9\n", size);
    return kernel_break_;
  }
}

Thread* KernelMemoryManager::KMMLockHeldBy()
{
  return lock_.heldBy();
}

void KernelMemoryManager::lockKMM()
{
  assert(!(system_state == RUNNING) || PageManager::instance()->heldBy() != currentThread);
  lock_.acquire();
}

void KernelMemoryManager::unlockKMM()
{
  assert(!(system_state == RUNNING) || PageManager::instance()->heldBy() != currentThread);
  lock_.release();
}

Mutex& KernelMemoryManager::getKMMLock()
{
  return lock_;
}
