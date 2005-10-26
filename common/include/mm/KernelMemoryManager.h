//----------------------------------------------------------------------
//   $Id: KernelMemoryManager.h,v 1.10 2005/10/26 11:17:40 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: KernelMemoryManager.h,v $
//  Revision 1.9  2005/09/26 15:10:21  btittelbach
//  warnings fix
//
//  Revision 1.8  2005/09/26 14:58:05  btittelbach
//  doxyfication
//
//  Revision 1.7  2005/08/07 16:47:25  btittelbach
//  More nice synchronisation Experiments..
//  RaceCondition/kprintf_nosleep related ?/infinite memory write loop Error still not found
//  kprintfd doesn't use a buffer anymore, as output_bochs blocks anyhow, should propably use some arch-specific interface instead
//
//  Revision 1.6  2005/05/03 18:31:09  btittelbach
//  fix of evil evil MemoryManager Bug
//
//  Revision 1.5  2005/05/03 17:32:29  btittelbach
//  size umbenannt
//
//  Revision 1.4  2005/04/28 14:07:12  btittelbach
//  Merge von size und flag (hack)
//
//  Revision 1.3  2005/04/23 22:20:30  btittelbach
//  just stuff
//
//  Revision 1.2  2005/04/23 17:35:03  nomenquis
//  fixed buggy memory manager
//  (giving out the same memory several times is no good idea)
//
//  Revision 1.1  2005/04/23 16:01:15  btittelbach
//  Testing Version vom KMM
//
//  Revision 1.1  2005/04/22 20:12:09  btittelbach
//  Kernel Memory Manager mit leichten bugs
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


#include "arch_panic.h"
#include "mm/new.h"
#include "kernel/SpinLock.h"
//#include "../../../arch/common/include/assert.h"



//-----------------------------------------------------------
/// MallocSegment Class
///
/// This is a collection of 4 32bit values holding information about an allocated memory segment
/// it is used as a ListNode by the KernelMemoryManager and is placed immediatly in front
/// of an memory segment.
/// If by error, some code should write beyond it's allocated memory segment, it would
/// "surely"(read maybe) write into the next MallocSegment instance, therefore 
/// overwriting the marker_ by which we hope to detect such an error
class MallocSegment
{
public:
//-----------------------------------------------------------
/// MallocSegment ctor
/// 
/// @param *prev Pointer to the previous MallocSegment in the list
/// @param *next Pointer to the next MallocSegment in the list
/// @param size Number of Bytes the described segment is large 
///     (this + sizeof(MallocSegment) + size is usually the start of the next segment)
/// @param used describes if the segment is allocated or free
  MallocSegment(MallocSegment *prev, MallocSegment *next, size_t size, bool used)
  {
    prev_=prev;
    marker_= 0xdeadbeef;
    size_flag_=(size & 0x7FFFFFFF);  //size to max 2^31-1
    next_=next;
    if (used)
      size_flag_ |= 0x80000000; //this is the used flag

  }
//-----------------------------------------------------------
/// @return the size of the segment in bytes (maximum 2^31-1 bytes)
  size_t getSize() {return (size_flag_ & 0x7FFFFFFF);}
//-----------------------------------------------------------
/// @param size sets the size of the segment in bytes (maximum 2^31-1 bytes)
  void setSize(size_t size) 
  { 
    size_flag_ &= 0x80000000;
    size_flag_ |= (size & 0x7FFFFFFF);
  }
//-----------------------------------------------------------
/// @return true if the segment is allocated, false it it is unused 
  bool getUsed() {return (size_flag_ & 0x80000000);}
//-----------------------------------------------------------
/// @param used set the segment as used (=true) or free (=false)
  void setUsed(bool used) 
  {
    size_flag_ &= 0x7FFFFFFF;
    if (used)
      size_flag_ |= 0x80000000; //this is the used flag
  }

  uint32 marker_;// = 0xdeadbeef;
  MallocSegment *next_;// = NULL;
  MallocSegment *prev_;// = NULL;
  
private:
  size_t size_flag_; // = 0; //max size is 2^31-1
};

// note by Bernhard: was auch eine nette Idee wäre:
// Segmente als Baum verlinkt und nach Größe sortiert
// und gleichzeitig als doppelt verkettet Liste in Reihenfolge der Adresse sortiert
// damit findet man schnell ein Segment richtiger größe
// und kann auch leicht ein Loch mit dem davor oder danach mergen
// free'n muß dann halt umsortiern etc

extern void* kernel_end_address;



//-----------------------------------------------------------
/// KernelMemoryManager Class
///
/// This class is a singelton. It should be accessed only by KernelMemoryManager::instance()->....
/// 
/// The KernelMemoryManager manages the useably Memory.
///...
///...


class KernelMemoryManager
{
public:
//-----------------------------------------------------------
/// createMemoryManager is called by startup() and does exatly what it's name promises
///
/// @param start_address the first free memory address
/// @param end_address the last address where memory is accessible (i.e. pages mapped)
  static uint32 createMemoryManager(pointer start_address, pointer end_address);
  static KernelMemoryManager *instance() {return instance_;}

//-----------------------------------------------------------
/// allocateMemory is called by new
/// searches the MallocSegment-List for a free segment with size >= requested_size
/// @param requested_size number of bytes to allocate 
/// @return pointer to Memory Adress or 0 if Not Enough Memory
  pointer allocateMemory(size_t requested_size);

//-----------------------------------------------------------
/// freeMemory is called by delete
/// checks if the given address points to an actual memory segment and marks it unused
/// if possible it tries to merge with the free segments around it
/// @param virtual_address memory address that was originally returned by allocateMemory
/// @return true if segment was freed or false if address was wrong
  bool freeMemory(pointer virtual_address);
  

//-----------------------------------------------------------
/// reallocateMemory is not used anywhere as of now
/// its function is to resize an already allocated memory segment
/// in the worst case moving the entire segment somewhere else as a result.
/// WARNING: therefore, code mußt assume that the memory address will change after using reallocateMemory
/// @param virtual_address address of the segment to resize
/// @param new_size the new size (acts like free if size == 0)
/// @return the (possibly altered) pointer to resized memory segment or 0 if unable to resize
  pointer reallocateMemory(pointer virtual_address, size_t new_size);  

//-----------------------------------------------------------
/// called from startup() after the scheduler has been created and just
/// before the Interrupts are turned on
  void startUsingSyncMechanism() {use_spinlock_=true;}

  bool isKMMLockFree()
  {
    if (likely (use_spinlock_))
      return lock_.isFree();
    else
      return true;
  }  
private:
  
  //WARNING: we really have to own that memory from start to end, nothing must be there
	KernelMemoryManager(pointer start_address=(pointer) &kernel_end_address , pointer end_address=0x80400000);

  MallocSegment *findFreeSegment(size_t requested_size);
  void fillSegment(MallocSegment *this_one, size_t size);
  void freeSegment(MallocSegment *this_one);
  MallocSegment *getSegmentFromAddress(pointer virtual_address);
  bool mergeWithFollowingFreeSegment(MallocSegment *this_one);
  void downsizeSegment(MallocSegment *this_one, size_t new_size);

  MallocSegment* first_;  //first_ must _never_ be NULL
  MallocSegment* last_;
  pointer malloc_end_;

  void lockKMM() {
    if (likely (use_spinlock_))
      lock_.acquire();
  }
  void unlockKMM()
  {
    if (likely (use_spinlock_))
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
