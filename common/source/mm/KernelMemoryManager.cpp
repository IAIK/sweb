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
    prenew_assert(false && "you can not use KernelMemoryManager::instance before the PageManager is ready!");
  }
  return instance_;
}

KernelMemoryManager::KernelMemoryManager(size_t min_heap_pages, size_t max_heap_pages) : lock_("KMM::lock_")
{
  prenew_assert(instance_ == 0);
  instance_ = this;
  pointer start_address = ArchCommon::getFreeKernelMemoryStart();
  prenew_assert(((start_address) % PAGE_SIZE) == 0);
  base_break_ = start_address;
  kernel_break_ = start_address + min_heap_pages * PAGE_SIZE;
  reserved_min_ = min_heap_pages * PAGE_SIZE;
  reserved_max_ = max_heap_pages * PAGE_SIZE;
  debug(KMM, "Clearing initial heap pages\n");
  memset((void*)start_address, 0, min_heap_pages * PAGE_SIZE);
  first_ = last_ =  new ((void*)start_address) MallocSegment(0, 0, false);
  debug(KMM, "KernelMemoryManager::ctor, Heap starts at %x and initially ends at %x\n", start_address, start_address + min_heap_pages * PAGE_SIZE);
}

pointer KernelMemoryManager::allocateMemory(size_t requested_size)
{

  lockKMM();
  pointer ptr = internalAllocateMemory(requested_size);
  unlockKMM();
  return ptr;
}

pointer KernelMemoryManager::internalAllocateMemory(size_t requested_size)
{
  prenew_assert(this->KMMLockHeldBy() == currentThread);
  requested_size = calculateRealSegmentSize(requested_size);
  MallocSegment* new_segment = 0;
  if(!(new_segment = getFreeSegment(requested_size)))
  {
    new_segment = addSegment(requested_size);
  }
  pointer ptr = new_segment->getUserAdress();
  debug(KMM, "allocateMemory returns address: %x with size: %d for segment %x \n", ptr, requested_size, new_segment);
  for(size_t* current = (size_t*)ptr; (size_t)current < (size_t)ptr + (requested_size - sizeof(MallocSegment)); current++)
  {
    if(unlikely(*current != 0))
    {
      debug(KMM, "Read %x at %x\n", *current, current);
      assert(*current == 0 && "Memory corruption detected! Probably a write after delete!");
    }
  }
  return ptr;
}

pointer KernelMemoryManager::reallocateMemory(pointer virtual_address, size_t new_size)
{
  if(virtual_address == 0) return allocateMemory(new_size);
  lockKMM();
  MallocSegment *to_resize = (MallocSegment*)((size_t)virtual_address - sizeof(MallocSegment));
  prenew_assert(to_resize->validate() && "reallocateMemory: reallocated invalid segment");
  debug(KMM, "reallocating for %x\n", to_resize);
  size_t original_chunk_size = calculateSegmentSize(to_resize);
  size_t new_chunk_size = calculateRealSegmentSize(new_size);
  if(new_chunk_size == original_chunk_size)
  {
    //Nothing to do here
    unlockKMM();
    return virtual_address;
  }
  else if (new_chunk_size < original_chunk_size)
  {
    splitSegment(to_resize, new_chunk_size);
    debug(KMM, "reallocateMemory: shrinked segment\n", to_resize);
    unlockKMM();
    return virtual_address;
  }
  else
  {
    pointer new_ptr = internalAllocateMemory(new_size);
    prenew_assert(new_ptr && "reallocateMemory: could not allocate new segment");
    memcpy((void*)new_ptr, (void*)virtual_address, calculateSegmentSize(to_resize) - sizeof(MallocSegment));
    unlockKMM();
    freeMemory(virtual_address);
    debug(KMM, "reallocateMemory: moved segment\n", to_resize);
    return new_ptr;
  }
}

bool KernelMemoryManager::freeMemory(pointer virtual_address)
{
  if (virtual_address == 0 || virtual_address < ((pointer) first_) || virtual_address >= kernel_break_)
    return false;

  MallocSegment *to_delete = (MallocSegment *)((size_t) virtual_address - sizeof(MallocSegment));
  lockKMM();
  prenew_assert(to_delete != 0 && "free: delete null pointer?");
  prenew_assert(to_delete->validate());
  debug(KMM, "freeing segment %x\n", to_delete);
  to_delete->setUsed(false);

  MallocSegment *prev = to_delete->prev_;
  if(mergeSegments(to_delete->prev_, to_delete))
  {
    to_delete = prev;
  }
  mergeSegments(to_delete, to_delete->next_);
  //if it was the last set the brk

  if(to_delete == last_)
  {
    ssize_t size = -(calculateSegmentSize(to_delete));
    last_ = to_delete->prev_;
    linkSegments(last_, 0);
    if(to_delete == first_)
      first_ = last_ = 0;
    memset((void*)to_delete->getUserAdress(), 0, calculateSegmentSize(to_delete) - sizeof(MallocSegment));
    ksbrk(size);
  }
  else
  {
    memset((void*)to_delete->getUserAdress(), 0, calculateSegmentSize(to_delete) - sizeof(MallocSegment));
  }
  unlockKMM();
  return true;
}

size_t KernelMemoryManager::calculateSegmentSize(MallocSegment* to_calculate)
{
  prenew_assert((unsigned int)to_calculate && "calgulateSegmentSize: segment mustn't be 0!");
  prenew_assert(to_calculate->validate());

  size_t size;
  if(to_calculate->next_)
  {
    // Easy, no explanation here
    prenew_assert(to_calculate->next_->validate());
    size = (size_t)to_calculate->next_ - (size_t)to_calculate;
  }
  else
  {
    // If the chunk is the last junk we need the end of heap to calculate
    // the size. Never forget to maintain this value.
    size = (size_t)kernel_break_ - (size_t)to_calculate;
  }
  return size;
}

size_t KernelMemoryManager::calculateRealSegmentSize(size_t requested_size)
{
  size_t total_size = sizeof(MallocSegment) + requested_size;
  return (total_size + 15) & ~0xF;
}

MallocSegment* KernelMemoryManager::getFreeSegment(size_t size)
{
  MallocSegment *current = first_;
  while(current)
  {
    printSegment(current);
    prenew_assert(current->validate());
    if(!current->getUsed())
    {
      if(calculateSegmentSize(current) >= size)
      {
        //split up (doesn't split if not needed)
        splitSegment(current, size);
        current->setUsed(true);
        return current;
      }
    }
    current = current->next_;
  }
  return 0;
}

void KernelMemoryManager::splitSegment(MallocSegment* to_split, size_t left_chunk_size)
{
  prenew_assert(to_split->validate());
  size_t original_chunk_size = calculateSegmentSize(to_split);
  prenew_assert(original_chunk_size >= left_chunk_size && "splitChunk: Block is too small!");
  if(original_chunk_size == left_chunk_size)
    return;

  MallocSegment *right_segment = new ((void*)((size_t)to_split + (size_t)left_chunk_size)) MallocSegment(0, 0, false);
  linkSegments(right_segment, to_split->next_);
  linkSegments(to_split, right_segment);
  mergeSegments(right_segment, right_segment->next_);

  if(to_split == last_)
    last_ = right_segment;
}

void KernelMemoryManager::linkSegments(MallocSegment* leftSegment, MallocSegment* rightSegment)
{
  prenew_assert((leftSegment || rightSegment) && "linkSegments: both segments are null");
  if(leftSegment != 0)
  {
    prenew_assert(leftSegment->validate());
    leftSegment->next_ = rightSegment;
  }
  if(rightSegment != 0)
  {
    prenew_assert(rightSegment->validate());
    rightSegment->prev_ = leftSegment;
  }
}

bool KernelMemoryManager::mergeSegments(MallocSegment* leftSegment, MallocSegment* rightSegment)
{
  bool return_value = false;
  prenew_assert(!(leftSegment == 0 && rightSegment == 0) && "mergeChunks: at least one chunk needs to be not null!");
  if(leftSegment && !leftSegment->getUsed() && rightSegment && !rightSegment->getUsed())
  {
    if(last_ == rightSegment)
      last_ = leftSegment;

    linkSegments(leftSegment, rightSegment->next_);
    return_value = true;
  }
  return return_value;
}

MallocSegment* KernelMemoryManager::addSegment(size_t segment_size)
{
  //skip checking of chunk_size because it is only used in malloc
  MallocSegment* new_segment = new ((void*)ksbrk(segment_size)) MallocSegment(0, 0, true);
  prenew_assert(new_segment != 0 && "addSegment: ran out of kernel memory");
  if(first_ == 0)
  {
    if(last_ != 0)
    {
      prenew_assert(false && "addChunk: Last chunk isn't 0 although first_chunk is");
    }
    first_ = new_segment;
  }

  linkSegments(last_, new_segment);
  linkSegments(new_segment, 0);

  last_ = new_segment;

  return new_segment;
}

void KernelMemoryManager::printAllSegments()
{
  MallocSegment* current = first_;
  while(current != 0)
  {
    assert(current->validate());
    printSegment(current);
    current = current->next_;
  }
}

void KernelMemoryManager::printSegment(MallocSegment* segment)
{
  debug(KMM, "current: %x size: %d used: %d \n", segment, calculateSegmentSize(segment), segment->getUsed());
}

pointer KernelMemoryManager::ksbrk(ssize_t size)
{
  debug(KMM, "KernelMemoryManager::ksbrk(%d)\n", size);
  prenew_assert(base_break_ <= (size_t)kernel_break_ + size && "kernel heap break value corrupted");
  prenew_assert((reserved_max_ == 0 || ((kernel_break_ - base_break_) + size) <= reserved_max_) && "maximum kernel heap size reached");
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
        prenew_assert(pm_ready_ && "Kernel Heap should not be used before PageManager is ready");
        size_t new_page = PageManager::instance()->allocPPN();
        if(unlikely(new_page == 0))
        {
          debug(KMM, "KernelMemoryManager::ksbrk(%d)4\n", size);
          kprintfd("KernelMemoryManager::freeSegment: FATAL ERROR\n");
          kprintfd("KernelMemoryManager::freeSegment: no more physical memory\n");
          prenew_assert(new_page != 0 && "Kernel Heap is out of memory");
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
        prenew_assert(pm_ready_ && "Kernel Heap should not be used before PageManager is ready");
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
  assert((!(system_state == RUNNING) || PageManager::instance()->heldBy() != currentThread) && "You're abusing the PageManager lock");
  lock_.acquire();
}

void KernelMemoryManager::unlockKMM()
{
  assert((!(system_state == RUNNING) || PageManager::instance()->heldBy() != currentThread) && "You're abusing the PageManager lock");
  lock_.release();
}

SpinLock& KernelMemoryManager::getKMMLock()
{
  return lock_;
}
