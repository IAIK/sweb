#include "KernelMemoryManager.h"
#include "types.h"
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
#include "Stabs2DebugInfo.h"
#include "backtrace.h"
#include "ArchMulticore.h"
#include "offsets.h"
#include "SystemState.h"
#include "Thread.h"

extern Stabs2DebugInfo const* kernel_debug_info;

alignas(KernelMemoryManager) unsigned char kmm[sizeof(KernelMemoryManager)];

KernelMemoryManager* KernelMemoryManager::instance_;

bool KernelMemoryManager::kmm_ready_ = false;

KernelMemoryManager* KernelMemoryManager::instance()
{
  assert(instance_ && "you can not use KernelMemoryManager::instance before the PageManager is ready!");
  return instance_;
}

void KernelMemoryManager::init()
{
    assert(KernelMemoryManager::instance_ == nullptr);
    size_t max_heap_pages = calcNumHeapPages();
    size_t num_reserved_heap_pages = mapKernelHeap(max_heap_pages);
    KernelMemoryManager::instance_ = new (&kmm) KernelMemoryManager(num_reserved_heap_pages, max_heap_pages);
}

size_t KernelMemoryManager::calcNumHeapPages()
{
    size_t heap_pages = PageManager::instance().getNumFreePages()/2;
    if (heap_pages > 1024)
        heap_pages = 1024 + (heap_pages - 1024)/8;
    return heap_pages;
}

size_t KernelMemoryManager::mapKernelHeap(size_t max_heap_pages)
{
  debug(MAIN, "Before kernel heap allocation:\n");
  PageManager::instance().printUsageInfo();
  debug(MAIN, "Num free pages: %zu\n", PageManager::instance().getNumFreePages());

  debug(MAIN, "Mapping %zu kernel heap pages at [%p, %p)\n", max_heap_pages, (char*)ArchCommon::getFreeKernelMemoryStart(), (char*)ArchCommon::getFreeKernelMemoryStart() + max_heap_pages*PAGE_SIZE);
  size_t num_reserved_heap_pages = 0;
  for (size_t kheap_vpn = ArchCommon::getFreeKernelMemoryStart() / PAGE_SIZE; num_reserved_heap_pages < max_heap_pages; ++num_reserved_heap_pages, ++kheap_vpn)
  {
    ppn_t ppn_to_map = PageManager::instance().allocPPN();
    if(MAIN & OUTPUT_ADVANCED)
      debug(MAIN, "Mapping kernel heap vpn %p -> ppn %p\n", (void*)kheap_vpn, (void*)ppn_to_map);
    assert(ArchMemory::mapKernelPage(kheap_vpn, ppn_to_map, true));
  }

  return num_reserved_heap_pages;
}


KernelMemoryManager::KernelMemoryManager(size_t min_heap_pages, size_t max_heap_pages) :
        tracing_(false), lock_("KMM::lock_"), segments_used_(0), segments_free_(0)
{
  debug(KMM, "Initializing KernelMemoryManager\n");

  pointer start_address = ArchCommon::getFreeKernelMemoryStart();
  assert(((start_address) % PAGE_SIZE) == 0);
  base_break_ = start_address;
  kernel_break_ = start_address + min_heap_pages * PAGE_SIZE;
  reserved_min_ = min_heap_pages * PAGE_SIZE;
  reserved_max_ = max_heap_pages * PAGE_SIZE;
  debug(KMM, "Reserved min: %#zx pages (%zu bytes), reserved max: %#zx pages (%zu bytes)\n", min_heap_pages, reserved_min_, max_heap_pages, reserved_max_);

  debug(KMM, "Clearing initial heap pages [%p, %p)\n", (void*)start_address, (char*)start_address + min_heap_pages * PAGE_SIZE);
  memset((void*)start_address, 0, min_heap_pages * PAGE_SIZE);

  first_ = (MallocSegment*)start_address;
  new (first_) MallocSegment(nullptr, nullptr, min_heap_pages * PAGE_SIZE - sizeof(MallocSegment), false);
  last_ = first_;

  kmm_ready_ = true;

  debug(KMM, "KernelMemoryManager::ctor, Heap starts at %zx and initially ends at %zx\n", start_address, start_address + min_heap_pages * PAGE_SIZE);
}

bool KernelMemoryManager::isReady()
{
    return kmm_ready_;
}


pointer KernelMemoryManager::allocateMemory(size_t requested_size, pointer called_by)
{
  debug(KMM, "allocateMemory, size: %zu, called by: %zx\n", requested_size, called_by);
  assert((requested_size & 0x80000000) == 0 && "requested too much memory");

  // 16 byte alignment
  requested_size = (requested_size + 0xF) & ~0xF;

  lockKMM();
  pointer ptr = private_AllocateMemory(requested_size, called_by);
  unlockKMM();

  debug(KMM, "allocateMemory returns address: %zx \n", ptr);
  return ptr;
}


pointer KernelMemoryManager::private_AllocateMemory(size_t requested_size, pointer called_by)
{
  assert(Thread::currentThreadIsStackCanaryOK() && "Kernel stack corruption detected.");
  debugAdvanced(KMM, "private_allocateMemory, size: %zu, called by: %zx\n", requested_size, called_by);
  assert((requested_size & 0xF) == 0 && "Attempt to allocate block with unaligned size");

  // find next free pointer of neccessary size + sizeof(MallocSegment);
  MallocSegment *new_pointer = findFreeSegment(requested_size);

  if (new_pointer == nullptr)
  {
    kprintfd("KernelMemoryManager::allocateMemory: Not enough Memory left\n");
    kprintfd("Are we having a memory leak in the kernel??\n");
    kprintfd(
        "This might as well be caused by running too many threads/processes, which partially reside in the kernel.\n");
    assert(false && "Kernel Heap is out of memory");
    return 0;
  }

  fillSegment(new_pointer, requested_size);
  new_pointer->freed_at_ = 0;
  new_pointer->alloc_at_ = tracing_ ? called_by : 0;
  new_pointer->alloc_by_ = (pointer)currentThread;

  return ((pointer) new_pointer) + sizeof(MallocSegment);
}

bool KernelMemoryManager::freeMemory(pointer virtual_address, pointer called_by)
{
  assert(Thread::currentThreadIsStackCanaryOK() && "Kernel stack corruption detected.");
  if (virtual_address == 0)
    return false;

  assert((virtual_address >= ((pointer) first_)) && (virtual_address < kernel_break_) && "Invalid free of address not in kernel heap");

  lockKMM();

  MallocSegment *m_segment = getSegmentFromAddress(virtual_address);
  m_segment->checkCanary();

  freeSegment(m_segment, called_by);
  if ((pointer)m_segment < kernel_break_ && m_segment->markerOk())
    m_segment->freed_at_ = called_by;

  unlockKMM();
  return true;
}


pointer KernelMemoryManager::reallocateMemory(pointer virtual_address, size_t new_size, pointer called_by)
{
  assert(Thread::currentThreadIsStackCanaryOK() && "Kernel stack corruption detected.");
  debug(KMM, "realloc %p, new size: %zu, calledbefore(1): %zx\n", (void*)virtual_address, new_size, called_by);
  assert((new_size & 0x80000000) == 0 && "requested too much memory");
  if (new_size == 0)
  {
    //in case you're wondering: we really don't want to lock here yet :) guess why
    freeMemory(virtual_address, called_by);
    return 0;
  }
  //iff the old segment is no segment ;) -> we create a new one
  if (virtual_address == 0)
    return allocateMemory(new_size, called_by);

  // 16 byte alignment
  new_size = (new_size + 0xF) & ~0xF;
  assert((new_size & 0xF) == 0 && "BUG: segment size must be aligned");

  lockKMM();

  MallocSegment *m_segment = getSegmentFromAddress(virtual_address);
  m_segment->checkCanary();
  assert(m_segment->getUsed());

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
    if (m_segment->next_ && !m_segment->next_->getUsed() && (m_segment->getSize() + sizeof(*m_segment->next_) + m_segment->next_->getSize() >= new_size))
    {
      auto s = mergeSegments(m_segment, m_segment->next_);
      fillSegment(s, new_size, 0);
      assert(s == m_segment);
      assert(m_segment->getSize() >= new_size);
      assert(s->getUsed());
      unlockKMM();
      return virtual_address;
    }

    //or not.. lets search for larger space

    //thx to Philipp Toeglhofer we are not going to deadlock here anymore ;)
    pointer new_address = private_AllocateMemory(new_size, called_by);
    if (new_address == 0)
    {
      //we are not freeing the old semgent in here, so that the data is not
      //getting lost, although we could not allocate more memory

      kprintfd("KernelMemoryManager::reallocateMemory: Not enough Memory left\n");
      kprintfd("Are we having a memory leak in the kernel??\n");
      kprintfd(
          "This might as well be caused by running too many threads/processes, which partially reside in the kernel.\n");
      assert(false && "Kernel Heap is out of memory");
      unlockKMM();
      return 0;
    }
    memcpy((void*) new_address, (void*) virtual_address, m_segment->getSize());
    freeSegment(m_segment, called_by);
    if ((pointer)m_segment < kernel_break_ && m_segment->markerOk())
      m_segment->freed_at_ = called_by;
    unlockKMM();
    return new_address;
  }
}


MallocSegment* KernelMemoryManager::getSegmentFromAddress(pointer virtual_address) const
{
  MallocSegment *m_segment;
  m_segment = (MallocSegment*) (virtual_address - sizeof(MallocSegment));
  assert(virtual_address != 0 && m_segment != nullptr && "trying to access a nullpointer");
  m_segment->checkCanary();
  return m_segment;
}


MallocSegment *KernelMemoryManager::findFreeSegment(size_t requested_size)
{
  debugAdvanced(KMM, "findFreeSegment: seeking memory block of bytes: %zd \n",
                requested_size + sizeof(MallocSegment));

  MallocSegment *current = first_;
  while (current)
  {
    debugAdvanced(KMM, "findFreeSegment: current: %p size: %zd used: %d \n",
                  current, current->getSize() + sizeof(MallocSegment), current->getUsed());

    current->checkCanary();
    if ((current->getSize() >= requested_size) && !current->getUsed())
      return current;

    current = current->next_;
  }

  // No free segment found, could we allocate more memory?
  if(last_->getUsed())
  {
    // In this case we have to create a new segment...
    MallocSegment* new_segment = new ((void*)ksbrk(sizeof(MallocSegment) + requested_size)) MallocSegment(last_, nullptr, requested_size, false);
    last_->next_ = new_segment;
    last_ = new_segment;
    assert(new_segment->getSize() == requested_size);
    assert(!new_segment->getUsed());
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
  if(KMM & OUTPUT_ADVANCED)
          debug(KMM, "fillSegment, %p, size: %zu, zero_check: %u\n", this_one, requested_size, zero_check);

  assert(this_one && "trying to access a nullpointer");
  this_one->checkCanary();
  const size_t size = this_one->getSize();
  assert(size >= requested_size && "segment is too small for requested size");
  assert((requested_size & 0xF) == 0 && "Attempt to fill segment with unaligned size");

  // TODO: Rest of free memory is in one big (> multiple megabytes) unused chunk at the end that will be completely checked
  // and set to zero _every time_ a fresh chunk is allocated/split off from the big chunk. This is slow as hell.
  size_t* mem = (size_t*) (this_one + 1);
  uint8* memb = (uint8*)mem;
  // sizeof(size_t) steps
  if (zero_check)
  {
    for (size_t i = 0, steps = size / sizeof(*mem); i < steps; ++i)
    {
      if(unlikely(mem[i] != 0))
      {
        debugAlways(KMM, "KernelMemoryManager::fillSegment: WARNING: Memory not zero at %p (value=%zx)\n", mem + i, mem[i]);
        if(this_one->freed_at_)
        {
          if(kernel_debug_info)
          {
            debugAlways(KMM, "KernelMemoryManager::freeSegment: The chunk may previously be freed at: ");
            kernel_debug_info->printCallInformation(this_one->freed_at_);
          }
          assert(false);
        }
        mem[i] = 0;
      }
    }
    // handle remaining bytes
    for(size_t i = size - (size % sizeof(*mem)); i < size; ++i)
    {
      if(unlikely(memb[i] != 0))
      {
        debugAlways(KMM, "KernelMemoryManager::fillSegment: WARNING: Memory not zero at %p (value=%x)\n", memb + i, memb[i]);
        if(this_one->freed_at_)
        {
          if(kernel_debug_info)
          {
            debugAlways(KMM, "KernelMemoryManager::freeSegment: The chunk may previously be freed at: ");
            kernel_debug_info->printCallInformation(this_one->freed_at_);
          }
          assert(false);
        }
        mem[i] = 0;
      }
    }
  }

  //size stays as it is, if there would be no more space to add a new segment
  this_one->setUsed(true);
  assert(this_one->getUsed() == true && "trying to fill an unused segment");

  //add a free segment after this one, if there's enough space
  size_t space_left = this_one->getSize() - requested_size;
  if (space_left > sizeof(MallocSegment))
  {
    this_one->setSize(requested_size);
    assert(this_one->getSize() == requested_size && "size error");
    assert(this_one->getUsed() == true && "trying to fill an unused segment");

    MallocSegment *new_segment =
            new ((void*) (((pointer)(this_one + 1)) + requested_size)) MallocSegment(
                    this_one, this_one->next_, space_left - sizeof(MallocSegment), false);

    assert(this_one->next_ || (this_one == last_));

    if (this_one->next_)
    {
      this_one->next_->checkCanary();
      this_one->next_->prev_ = new_segment;
    }
    else
    {
      last_ = new_segment;
    }

    this_one->next_ = new_segment;
    assert(new_segment->next_ || (new_segment == last_));
  }

  debug(KMM, "fillSegment: filled memory block of bytes: %zd \n", this_one->getSize() + sizeof(MallocSegment));
}


void KernelMemoryManager::freeSegment(MallocSegment *this_one, pointer called_by)
{
  debug(KMM, "KernelMemoryManager::freeSegment(%p)\n", this_one);
  assert(this_one && "trying to access a nullpointer");
  this_one->checkCanary();

  if (!this_one->getUsed())
  {
    kprintfd("KernelMemoryManager::freeSegment: FATAL ERROR\n");
    kprintfd("KernelMemoryManager::freeSegment: tried freeing not used memory block\n");
    if(this_one->freed_at_)
    {
      if(kernel_debug_info)
      {
        kprintfd("KernelMemoryManager::freeSegment: The chunk may previously be freed at: ");
        kernel_debug_info->printCallInformation(this_one->freed_at_);
      }
    }
    assert(false);
  }

  debug(KMM, "freeSegment: freeing block: %p of bytes: %zd \n",
        this_one, this_one->getSize() + sizeof(MallocSegment));

  this_one->setUsed(false);
  this_one->freed_at_ = called_by;

  this_one = mergeSegments(this_one, this_one->prev_);
  this_one = mergeSegments(this_one, this_one->next_);

  memset((void*)(this_one + 1), 0, this_one->getSize()); // ease debugging

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
        assert(this_one && this_one->prev_);
        this_one->prev_->checkCanary();
        this_one->prev_->next_ = nullptr;
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
    while (current)
    {
      if(KMM & OUTPUT_ADVANCED)
              debug(KMM, "freeSegment: current: %p prev: %p next: %p size: %zd used: %d\n", current, current->prev_,
            current->next_, current->getSize() + sizeof(MallocSegment), current->getUsed());
      current->checkCanary();
      current = current->next_;
    }
  }
}

pointer KernelMemoryManager::ksbrk(ssize_t size)
{
  assert(base_break_ <= (size_t)kernel_break_ + size && "kernel heap break value corrupted");

  if (!(((kernel_break_ - base_break_) + size) <= reserved_max_))
  {
      debugAlways(KMM, "Used kernel memory: %zu\n", getUsedKernelMemory(true));
      assert(false && "maximum kernel heap size reached");
  }
  assert(DYNAMIC_KMM && "ksbrk should only be called if DYNAMIC_KMM is 1 - not in baseline SWEB");
  if(size != 0)
  {
    size_t old_brk = kernel_break_;
    size_t cur_top_vpn = kernel_break_ / PAGE_SIZE;
    if ((kernel_break_ % PAGE_SIZE) == 0)
      cur_top_vpn--;
    kernel_break_ = ((size_t)kernel_break_) + size;
    size_t new_top_vpn = (kernel_break_ )  / PAGE_SIZE;
    if ((kernel_break_ % PAGE_SIZE) == 0)
      new_top_vpn--;
    if(size > 0)
    {
      debug(KMM, "%zx != %zx\n", cur_top_vpn, new_top_vpn);
      while(cur_top_vpn != new_top_vpn)
      {
        debug(KMM, "%zx != %zx\n", cur_top_vpn, new_top_vpn);
        cur_top_vpn++;
        size_t new_page = PageManager::instance().allocPPN();
        if(unlikely(new_page == 0))
        {
          debug(KMM, "KernelMemoryManager::freeSegment: FATAL ERROR\n");
          debug(KMM, "KernelMemoryManager::freeSegment: no more physical memory\n");
          assert(new_page != 0 && "Kernel Heap is out of memory");
        }
        debug(KMM, "kbsrk: map %zx -> %zx\n", cur_top_vpn, new_page);
        assert(ArchMemory::mapKernelPage(cur_top_vpn, new_page));
      }

    }
    else
    {
      while(cur_top_vpn != new_top_vpn)
      {
        ArchMemory::unmapKernelPage(cur_top_vpn);
        cur_top_vpn--;
      }
    }
    return old_brk;
  }
  else
  {
    return kernel_break_;
  }
}


Thread* KernelMemoryManager::KMMLockHeldBy() const
{
  return lock_.heldBy();
}


void KernelMemoryManager::lockKMM()
{
  if(system_state == RUNNING)
  {
    debug(KMM, "CPU %zx lock KMM\n", SMP::currentCpuId());
  }
  assert(((system_state != RUNNING) || ArchInterrupts::testIFSet()) && "Cannot allocate heap memory/lock KMM while interrupts are disabled");
  lock_.acquire(getCalledBefore(1));
}


void KernelMemoryManager::unlockKMM()
{
  if(system_state == RUNNING)
  {
          debug(KMM, "CPU %zx unlock KMM\n", SMP::currentCpuId());
  }
  lock_.release(getCalledBefore(1));
}


Mutex& KernelMemoryManager::getKMMLock()
{
  return lock_;
}


size_t KernelMemoryManager::getUsedKernelMemory(bool show_allocs = false) {
    MallocSegment *current = first_;
    size_t size = 0, blocks = 0, unused = 0;
    if(show_allocs) kprintfd("Kernel Memory Usage\n\n");
    while (current)
    {
      if (current->getUsed()) {
        size += current->getSize();
        blocks++;
        if(current->alloc_at_ && show_allocs)
        {
            if(kernel_debug_info)
            {
                kprintfd("%8zu bytes (by %p) at: ", current->getSize(), (void*)current->alloc_by_);
                kernel_debug_info->printCallInformation(current->alloc_at_);
            }
        }
      } else {
          unused += current->getSize();
      }

      current = current->next_;
    }
    if(show_allocs) kprintfd("\n%zu bytes in %zu blocks are in use (%zu%%)\n", size, blocks, 100 * size / (size + unused));
    return size;
}


void KernelMemoryManager::startTracing() {
    tracing_ = true;
}


void KernelMemoryManager::stopTracing() {
    tracing_ = false;
}


bool MallocSegment::checkCanary()
{
  if (!markerOk())
  {
    kprintfd("Memory corruption in KMM segment %p, size: %zx, marker: %" PRIx64 " at %p, alloc at: %zx, freed at: %zx\n",
             this, getSize(), marker_, &marker_, alloc_at_, freed_at_);
    if(freed_at_)
    {
      if(kernel_debug_info)
      {
        kprintfd("The segment may have previously been freed at: ");
        kernel_debug_info->printCallInformation(freed_at_);
      }
    }
    assert(false && "memory corruption - probably 'write after delete'");
  }
  return true;
}



MallocSegment* KernelMemoryManager::mergeSegments(MallocSegment* s1, MallocSegment* s2)
{
        assert(s1);
        s1->checkCanary();
        if(s2) s2->checkCanary();

        // Only merge if the segment we want to merge with is not in use
        if(!s2 || (s1 == s2) || s2->getUsed())
                return s1;

        if(s2 < s1)
        {
                MallocSegment* tmp = s1;
                s1 = s2;
                s2 = tmp;
        }

        // Can merge a used segment with an unused one following it but not the other way around
        assert(!s2->getUsed());

        size_t s2_true_size = (s2->next_ ? (pointer)s2->next_ : kernel_break_) - (pointer)s2;
        debug(KMM, "mergeSegments %p [%zu] used: %u + %p [%zu] used: %u => %p [%zu] used %u\n",
              s1, s1->getSize() + sizeof(*s1), s1->getUsed(), s2, s2->getSize() + sizeof(*s2), s2->getUsed(),
              s1, sizeof(*s1) + s1->getSize() + s2_true_size, s1->getUsed());


        assert(s1->next_ == s2);
        assert(((pointer)(s1+1) + s1->getSize()) == (pointer)s2);


        s1->setSize(s1->getSize() + s2_true_size);
        s1->next_ = s2->next_;
        if(s2->next_)
        {
                s2->next_->checkCanary();
                s2->next_->prev_ = s1;
        }
        else
        {
                assert(s2 == last_ && "this should never happen, there must be a bug in KMM");
                last_ = s1;
        }

        memset(s2, 0, sizeof(*s2));

        return s1;
}

pointer KernelMemoryManager::getKernelBreak() const {
  return kernel_break_;
}

pointer KernelMemoryManager::getKernelHeapStart() const {
  return base_break_;
}

pointer KernelMemoryManager::getKernelHeapMaxEnd() const {
  return base_break_ + reserved_max_;
}
