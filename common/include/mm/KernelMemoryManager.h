//----------------------------------------------------------------------
//   $Id: KernelMemoryManager.h,v 1.2 2005/04/23 17:35:03 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: KernelMemoryManager.h,v $
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
    size_=size;
    next_=next;
    flag_=((used)?1:0);
    marker_= 0xdeadbeef;
  }
  uint32 marker_;// = 0xdeadbeef;
  MallocSegment *next_;// = NULL;
  MallocSegment *prev_;// = NULL;
  uint8 flag_;// = 0;
  size_t size_;// = 0;

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
  
  void memoryCopy(pointer source, pointer destination, size_t size);
  void memoryZero(pointer virtual_address, size_t size);
  
  
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
  //pointer *malloc_start_;
  pointer malloc_end_;

  //statistics:
  uint32 segments_used_;
  uint32 segments_free_;
  size_t memory_free_;

  static KernelMemoryManager *instance_;
  
};
