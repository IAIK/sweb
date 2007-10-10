/**
 * @file KernelMemoryManager.h
 * Management and Allocation of All that is Memory
 */

#ifndef KERNEL_MEMORY_MANAGER__
#define KERNEL_MEMORY_MANAGER__

#include "arch_panic.h"
#include "mm/new.h"
#include "kernel/SpinLock.h"

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
    MallocSegment ( MallocSegment *prev, MallocSegment *next, size_t size, bool used )
    {
      prev_=prev;
      marker_= 0xdeadbeef;
      size_flag_= ( size & 0x7FFFFFFF );  //size to max 2^31-1
      next_=next;
      if ( used )
        size_flag_ |= 0x80000000; //this is the used flag

    }
    /**
     * returns the size of the segment in bytes (maximum 2^31-1 bytes)
     * @return the size
     */
  size_t getSize() {return ( size_flag_ & 0x7FFFFFFF );}
    /**
     * sets the sizeof the segment in bytes (maximum 2^31-1 bytes)
     * @param size the size to set
     */
    void setSize ( size_t size )
    {
      size_flag_ &= 0x80000000;
      size_flag_ |= ( size & 0x7FFFFFFF );
    }
    /**
     * checks if the segment is allocated
     * @return true if the segment is allocated, false it it is unused
     */
    bool getUsed() {return ( size_flag_ & 0x80000000 );}

    /**
     * sets the segment as used (=true) or free (=false)
     * @param used used (=true) or free (=false)
     */
    void setUsed ( bool used )
    {
      size_flag_ &= 0x7FFFFFFF;
      if ( used )
        size_flag_ |= 0x80000000; //this is the used flag
    }

    uint32 marker_;// = 0xdeadbeef;
    MallocSegment *next_;// = NULL;
    MallocSegment *prev_;// = NULL;

  private:
    size_t size_flag_; // = 0; //max size is 2^31-1
};

// note by Bernhard: was auch eine nette Idee waee:
// Segmente als Baum verlinkt und nach Groesze sortiert
// und gleichzeitig als doppelt verkettet Liste in Reihenfolge der Adresse sortiert
// damit findet man schnell ein Segment richtiger groesze
// und kann auch leicht ein Loch mit dem davor oder danach mergen
// free'n muss dann halt umsortiern etc

extern void* kernel_end_address;



/**
 * @class KernelMemoryManager
 * this class is a singelton. It should be accessed only by KernelMemoryManager::instance()->....
 * The KernelMemoryManager manages the useably Memory.
 */

class KernelMemoryManager
{
  public:
    /**
     * createMemoryManager is called by startup() and does exatly what it's name promises
     * @param start_address the first free memory address
     * @param end_address the last address where memory is accessible (i.e. pages mapped)
     */
    static uint32 createMemoryManager ( pointer start_address, pointer end_address );

    /**
     * the access method to the singleton instance
     * @return the instance
     */
    static KernelMemoryManager *instance() {return instance_;}

    /**
     * allocateMemory is called by new
     * searches the MallocSegment-List for a free segment with size >= requested_size
     * @param requested_size number of bytes to allocate
     * @return pointer to Memory Adress or 0 if Not Enough Memory
     */
    pointer allocateMemory ( size_t requested_size );

    /**
     * freeMemory is called by delete
     * checks if the given address points to an actual memory segment and marks it unused
     * if possible it tries to merge with the free segments around it
     * @param virtual_address memory address that was originally returned by allocateMemory
     * @return true if segment was freed or false if address was wrong
     */
    bool freeMemory ( pointer virtual_address );


    /**
     * reallocateMemory is not used anywhere as of now
     * its function is to resize an already allocated memory segment
     * in the worst case moving the entire segment somewhere else as a result.
     * WARNING: therefore, code must assume that the memory address will change after using reallocateMemory
     * @param virtual_address address of the segment to resize
     * @param new_size the new size (acts like free if size == 0)
     * @return the (possibly altered) pointer to resized memory segment or 0 if unable to resize
     */
    pointer reallocateMemory ( pointer virtual_address, size_t new_size );

    /**
     * called from startup() after the scheduler has been created and just
     * before the Interrupts are turned on
     */
    void startUsingSyncMechanism() {use_spinlock_=true;}

    /**
     * checks if the kernel memory manager lock is free
     * @return true if it is free
     */
    bool isKMMLockFree()
    {
      if ( likely ( use_spinlock_ ) )
        return lock_.isFree();
      else
        return true;
    }
  private:

    //WARNING: we really have to own that memory from start to end, nothing must be there
    /**
     * Constructor - private because its a singleton
     * @param start_address the start address of the memory
     * @param end_address the end address of the memory
     */
    KernelMemoryManager ( pointer start_address= ( pointer ) &kernel_end_address , pointer end_address=0x80400000 );

    /**
     * returns a free memory segment of the requested size
     * @param requested_size the size
     * @return the segment
     */
    MallocSegment *findFreeSegment ( size_t requested_size );

    /**
     * creates a new segment after the given one if the space is big enough
     * @param this_one the segment
     * @param size the size to used
     */
    void fillSegment ( MallocSegment *this_one, size_t size );

    /**
     * frees the given segment
     * @param this_one the segment
     */
    void freeSegment ( MallocSegment *this_one );

    /**
     * returns the segment the virtual address is pointing to
     * @param virtual_address the address
     * @return the segment
     */
    MallocSegment *getSegmentFromAddress ( pointer virtual_address );

    /**
     * merges the given segment with the following one
     * @param this_one the segmnet
     * @return true on success
     */
    bool mergeWithFollowingFreeSegment ( MallocSegment *this_one );

    MallocSegment* first_;  //first_ must _never_ be NULL
    MallocSegment* last_;
    pointer malloc_end_;

    /**
     * locks the kernel memory manager
     */
    void lockKMM()
    {
      if ( likely ( use_spinlock_ ) )
        lock_.acquire();
    }

    /**
     * unlocks the kernel memory manager
     */
    void unlockKMM()
    {
      if ( likely ( use_spinlock_ ) )
        lock_.release();
    }
    bool use_spinlock_;
    SpinLock lock_;

    //statistics:
    uint32 segments_used_;
    uint32 segments_free_;
    size_t approx_memory_free_;

    static KernelMemoryManager *instance_;

};

#endif
