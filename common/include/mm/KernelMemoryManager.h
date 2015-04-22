#ifndef KERNEL_MEMORY_MANAGER__
#define KERNEL_MEMORY_MANAGER__

#include "new.h"
#include "SpinLock.h"
#include "assert.h"
#include "debug.h"
#include "kprintf.h"

#define MAGIC_SEGMENT 0xDEADBEEF

/**
 * @class MallocSegment
 *
 * This is a collection of 4 32bit values holding information about an allocated memory segment
 * it is used as a ListNode by the KernelMemoryManager and is placed immediatly in front
 * of an memory segment.
 * If by error, some code should write beyond it's allocated memory segment, it would
 * "surely"(read maybe) write into the next MallocSegment instance, therefore
 * overwriting the marker_ by which we hope to detect such an error
 */
class MallocSegment
{
  public:
    /**
     * Constructor
     * @param *prev Pointer to the previous MallocSegment in the list
     * @param *next Pointer to the next MallocSegment in the list
     * @param used describes if the segment is allocated or free
     */
    MallocSegment(MallocSegment *prev, MallocSegment *next, bool used)
    {
      marker_ = MAGIC_SEGMENT;
      prev_ = prev;
      next_ = next;
      flag_ = used;
    }

    /**
     * checks if the segment is allocated
     * @return true if the segment is allocated, false it it is unused
     */
    bool getUsed()
    {
      return flag_;
    }

    /**
     * sets the segment as used (=true) or free (=false)
     * @param used used (=true) or free (=false)
     */
    void setUsed(bool used)
    {
      flag_ = used;
    }

    pointer getUserAdress()
    {
    	return (pointer)((size_t)this + sizeof(MallocSegment));
    }

    bool validate()
    {
      if(unlikely(marker_ != MAGIC_SEGMENT))
      {
        debug(KMM, "marker for %x was %x instead of 0xdeadbeef\n", this, marker_);
        return false;
      }
      return true;
    }

    uint32 marker_;       // = 0xdeadbee0;
    uint32 flag_;
    MallocSegment *next_; // = NULL;
    MallocSegment *prev_; // = NULL;
};

extern void* kernel_end_address;

class KernelMemoryManager
{
  public:
    static KernelMemoryManager *instance();

    /**
     * allocateMemory is called by new
     * searches the MallocSegment-List for a free segment with size >= requested_size
     * @param requested_size number of bytes to allocate
     * @return pointer to Memory Address or 0 if Not Enough Memory
     */
    pointer allocateMemory(size_t requested_size);

    /**
     * freeMemory is called by delete
     * checks if the given address points to an actual memory segment and marks it unused
     * if possible it tries to merge with the free segments around it
     * @param virtual_address memory address that was originally returned by allocateMemory
     * @return true if segment was freed or false if address was wrong
     */
    bool freeMemory(pointer virtual_address);

    /**
     * reallocateMemory is not used anywhere as of now
     * its function is to resize an already allocated memory segment
     * in the worst case moving the entire segment somewhere else as a result.
     * WARNING: therefore, code must assume that the memory address will change after using reallocateMemory
     * @param virtual_address address of the segment to resize
     * @param new_size the new size (acts like free if size == 0)
     * @return the (possibly altered) pointer to resized memory segment or 0 if unable to resize
     */
    pointer reallocateMemory(pointer virtual_address, size_t new_size);

    SpinLock& getKMMLock();

    Thread* KMMLockHeldBy();

    KernelMemoryManager() : first_(0), last_(0), base_break_(0), kernel_break_(0), reserved_max_(0),
        reserved_min_(0), lock_(0) { assert(false && "dummy constructor - do not use!"); };

  protected:
    friend class PageManager;

    KernelMemoryManager(size_t min_heap_pages, size_t max_heap_pages);

    static size_t pm_ready_;

    static KernelMemoryManager *instance_;

  private:

    void freeSegment(MallocSegment *this_one);

    pointer internalAllocateMemory(size_t requested_size);
    size_t calculateSegmentSize(MallocSegment* to_calculate);
    size_t calculateRealSegmentSize(size_t requested_size);
    MallocSegment* getFreeSegment(size_t size);
    void splitSegment(MallocSegment* to_split, size_t left_chunk_size);
    void linkSegments(MallocSegment* leftSegment, MallocSegment* rightSegment);
    bool mergeSegments(MallocSegment* leftSegment, MallocSegment* rightSegment);
    MallocSegment* addSegment(size_t segment_size);
    void printAllSegments();
    void printSegment(MallocSegment* segment);

    pointer ksbrk(ssize_t size);

    MallocSegment* first_; //first_ must _never_ be NULL
    MallocSegment* last_;
    pointer base_break_;
    pointer kernel_break_;
    size_t reserved_max_;
    size_t reserved_min_;

    void lockKMM();
    void unlockKMM();

    SpinLock lock_;
};

#endif
