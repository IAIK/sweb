#ifndef KERNEL_MEMORY_MANAGER__
#define KERNEL_MEMORY_MANAGER__

#include "new.h"
#include "Mutex.h"
#include "assert.h"

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
     * @param size Number of Bytes the described segment is large
     *        (this + sizeof(MallocSegment) + size is usually the start of the next segment)
     * @param used describes if the segment is allocated or free
     */
    MallocSegment(MallocSegment *prev, MallocSegment *next, size_t size, bool used)
    {
      prev_ = prev;
      marker_ = 0xdeadbeef;
      size_flag_ = (size & 0x7FFFFFFF); //size to max 2^31-1
      next_ = next;
      if (used)
        size_flag_ |= 0x80000000; //this is the used flag

    }
    /**
     * returns the size of the segment in bytes (maximum 2^31-1 bytes)
     * @return the size
     */
    size_t getSize()
    {
      return (size_flag_ & 0x7FFFFFFF);
    }
    /**
     * sets the sizeof the segment in bytes (maximum 2^31-1 bytes)
     * @param size the size to set
     */
    void setSize(size_t size)
    {
      size_flag_ &= 0x80000000;
      size_flag_ |= (size & 0x7FFFFFFF);
    }
    /**
     * checks if the segment is allocated
     * @return true if the segment is allocated, false it it is unused
     */
    bool getUsed()
    {
      return (size_flag_ & 0x80000000);
    }

    /**
     * sets the segment as used (=true) or free (=false)
     * @param used used (=true) or free (=false)
     */
    void setUsed(bool used)
    {
      size_flag_ &= 0x7FFFFFFF;
      if (used)
        size_flag_ |= 0x80000000; //this is the used flag
    }

    uint32 marker_; // = 0xdeadbeef;
    MallocSegment *next_; // = NULL;
    MallocSegment *prev_; // = NULL;

  private:
    size_t size_flag_; // = 0; //max size is 2^31-1
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

    Mutex& getKMMLock();

    Thread* KMMLockHeldBy();

    KernelMemoryManager() : lock_(0) { assert(false && "dummy constructor - do not use!"); };

  protected:
    friend class PageManager;

    KernelMemoryManager(size_t min_heap_pages, size_t max_heap_pages);

    static size_t pm_ready_;

    static KernelMemoryManager *instance_;

  private:

    /**
     * returns a free memory segment of the requested size
     * @param requested_size the size
     * @return the segment
     */
    MallocSegment *findFreeSegment(size_t requested_size);

    /**
     * creates a new segment after the given one if the space is big enough
     * @param this_one the segment
     * @param size the size to used
     * @param zero_check whether memory is zero'd
     */
    void fillSegment(MallocSegment *this_one, size_t size, uint32 zero_check = 1);

    void freeSegment(MallocSegment *this_one);

    /**
     * returns the segment the virtual address is pointing to
     * @param virtual_address the address
     * @return the segment
     */
    MallocSegment *getSegmentFromAddress(pointer virtual_address);

    /**
     * merges the given segment with the following one
     * @param this_one the segmnet
     * @return true on success
     */
    bool mergeWithFollowingFreeSegment(MallocSegment *this_one);

    /**
     * This really implements the allocateMemory behaviour, but 
     * does not lock the KMM, so we can also use it within the
     * reallocate method
     */
    inline pointer private_AllocateMemory(size_t requested_size);

    pointer ksbrk(ssize_t size);

    MallocSegment* first_; //first_ must _never_ be NULL
    MallocSegment* last_;
    pointer base_break_;
    pointer kernel_break_;
    size_t reserved_max_;
    size_t reserved_min_;

    void lockKMM();
    void unlockKMM();

    Mutex lock_;

    uint32 segments_used_;
    uint32 segments_free_;
    size_t approx_memory_free_;
};

#endif
