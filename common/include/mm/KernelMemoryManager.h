//----------------------------------------------------------------------
//   $Id: KernelMemoryManager.h,v 1.6 2005/05/03 18:31:09 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: KernelMemoryManager.h,v $
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
//#include "../../../arch/common/include/assert.h"

// this will be written to the start of every mallog segment,
// god help us, if someone ever writes beyond his allocated memory
class MallocSegment
{
public:
  MallocSegment(MallocSegment *prev, MallocSegment *next, size_t size, bool used)
  {
    prev_=prev;
    marker_= 0xdeadbeef;
    size_flag_=(size & 0x7FFFFFFF);  //size to max 2^31-1
    next_=next;
    if (used)
      size_flag_ |= 0x80000000; //this is the used flag

  }

  size_t getSize() {return (size_flag_ & 0x7FFFFFFF);}
  void setSize(size_t size) 
  { 
    size_flag_ &= 0x80000000;
    size_flag_ |= (size & 0x7FFFFFFF);
  }
  bool getUsed() {return (size_flag_ & 0x80000000);}
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

class KernelMemoryManager
{
public:

  static uint32 createMemoryManager(pointer start_address, pointer end_address);
  static KernelMemoryManager *instance() {return instance_;}

  /// @return pointer to Memory Adress or -1 on NotEnoughMemory (?)
  pointer allocateMemory(size_t requested_size);
  
  /// @return bool false on NotEnoughMemory (?) or Overlap
  /// this won't write a MallocSegmet, but irreversible mark a memory segment as used
  //bool reserveMemory(pointer* virtual_address, size_t size);

  // free memory segment
  bool freeMemory(pointer virtual_address);
  
  //in worst case: memcpy the entire stuff somewhere else
  //WARNING: after MemCpy can't assume that pointer remains the same
  //return -1 if unable to resize
  //if size == 0 -> free(what_and_where_is_it);
  pointer reallocateMemory(pointer virtual_address, size_t new_size);  
  
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

  //statistics:
  uint32 segments_used_;
  uint32 segments_free_;
  size_t approx_memory_free_;

  static KernelMemoryManager *instance_;
  
};
