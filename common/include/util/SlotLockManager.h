/**
 * Filename: SlotLockManager.h
 * Description:
 *
 * Created on: 09.08.2012
 * Author: chris
 */

#ifndef SLOTLOCKMANAGER_H_
#define SLOTLOCKMANAGER_H_

#ifndef NO_USE_OF_MULTITHREADING
#include "Mutex.h"
#include "MutexLock.h"
#endif

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
#include "ustl/umap.h"
#define STL_NAMESPACE_PREFIX    ustl::
#else
#include <map>
#include "assert.h"
#define STL_NAMESPACE_PREFIX    std::
#endif

#include "fs/FileSystemLock.h"
#include "fs/FsDefinitions.h"

/**
 * @class provides an efficient way to lock (protect) certain slots in a system
 * with an arbitrary number of slots.
 */
template<class T, typename U> class SlotLockManager
{
public:
  /**
   * constructor
   */
  SlotLockManager();

  /**
   * destructor
   */
  virtual ~SlotLockManager()
  { assert(active_locks_.size() == 0); }

  /**
   * acquires a lock to the given Slot
   *
   * @param slot
   */
  void acquireReadBlocking(const T& slot)
  {
#ifndef NO_USE_OF_MULTITHREADING
    MutexLock mutex_lock(lock_map_lock_);
#endif

    FileSystemLock* lock = findLockInMap(slot, 1);
    if(lock == NULL)
    {
      lock = createNewSlot(slot);
    }

    assert(lock != NULL);
    lock->acquireReadBlocking();
  }

  bool acquireReadNonBlocking(const T& slot)
  {
#ifndef NO_USE_OF_MULTITHREADING
    MutexLock mutex_lock(lock_map_lock_);
#endif

    FileSystemLock* lock = findLockInMap(slot, 1);
    if(lock == NULL)
    {
      lock = createNewSlot(slot);
    }

    assert(lock != NULL);
    return lock->acquireReadNonBlocking();
  }

  /**
   * releases a previous read-acquired slot
   * @param slot
   */
  void releaseRead(const T& slot)
  {
#ifndef NO_USE_OF_MULTITHREADING
    MutexLock mutex_lock(lock_map_lock_);
#endif

    FileSystemLock* lock = findLockInMap(slot, -1);
    assert(lock != NULL); // !!! indicates a fatal locking fault
    lock->releaseRead();
    removeEntryIfUnused(slot);
  }

  /**
   * acquires a write-lock to the given Slot
   * @param slot
   */
  void acquireWriteBlocking(const T& slot)
  {
#ifndef NO_USE_OF_MULTITHREADING
    MutexLock mutex_lock(lock_map_lock_);
#endif

    FileSystemLock* lock = findLockInMap(slot, 1);
    if(lock == NULL)
    {
      lock = createNewSlot(slot);
    }

    assert(lock != NULL);
    lock->acquireWriteBlocking();
  }

  bool acquireWriteNonBlocking(const T& slot)
  {
#ifndef NO_USE_OF_MULTITHREADING
    MutexLock mutex_lock(lock_map_lock_);
#endif

    FileSystemLock* lock = findLockInMap(slot, 1);
    if(lock == NULL)
    {
      lock = createNewSlot(slot);
    }

    assert(lock != NULL);
    return lock->acquireWriteNonBlocking();
  }

  /**
   * releases a previous write-acquired slot
   * @param slot
   */
  void releaseWrite(const T& slot)
  {
#ifndef NO_USE_OF_MULTITHREADING
    MutexLock mutex_lock(lock_map_lock_);
#endif

    FileSystemLock* lock = findLockInMap(slot, -1);
    assert(lock != NULL); // !!! indicates a fatal locking fault!
    lock->releaseWrite();
    removeEntryIfUnused(slot);
  }

  /**
   * setting the additional info field of a acquired slot-info structure
   *
   * @param slot the slot to set the additional info
   * @param info the additional informations to set
   * @return true if the information was successfully set; false if
   * there is non such a slot currently acquired
   */
  bool setSlotAdditionalInfo(const T& slot, U info);

  /**
   * getting the additional informations of a slot
   *
   * @return
   */
  U getSlotAdditionalInfo(const T& slot) const;

private:

  /**
   * finding a lock in the map
   * @param slot the slot to find the lock for
   * @param delta_ref_count what to do with the ref-count? (-1 dec, 0-stay, 1-incr)
   */
  FileSystemLock* findLockInMap(const T& slot, int32 delta_ref_count)
  {
    for(map_it it = active_locks_.begin(); it != active_locks_.end(); it++)
    {
      if((*it).first == slot)
      {
        // we found the lock :D
        if(delta_ref_count < 0)
          (*it).second.ref_count--;
        else if(delta_ref_count > 0)
          (*it).second.ref_count++;

        // returning the FsLock:
        return (*it).second.fs_lock;
      }
    }
    // no lock present in the map
    return NULL;
  }

  /**
   * creates and inserts a new slot into the map
   *
   * @param slot the id of the new slot to create
   * @return the new FileSystemLock object of the slot
   */
  FileSystemLock* createNewSlot(const T& slot)
  {
    // insert new lock to the map
    SlotLock slot_lock;
    slot_lock.fs_lock = FileSystemLock::getNewFSLock();
    slot_lock.ref_count = 1;
    slot_lock.additional_info = 0;

    active_locks_.insert(STL_NAMESPACE_PREFIX make_pair(slot, slot_lock));

    return slot_lock.fs_lock;
  }

  /**
   * deletes an lock-map entry if the ref-count is 0
   */
  void removeEntryIfUnused(const T& slot)
  {
    for(map_it it = active_locks_.begin(); it != active_locks_.end(); it++)
    {
      if((*it).first == slot)
      {
        if((*it).second.ref_count == 0)
        {
          delete (*it).second.fs_lock;
          active_locks_.erase(it);
        }
        return; // entry found, quit
      }
    }
  }

  /**
   * a Slot-Lock entry carrying informations
   */
  struct SlotLock
  {
    FileSystemLock* fs_lock;  // the Locking-aid to establish mutual exclusion
    uint32 ref_count;         // reference counter
    U additional_info;        // user defined custom additional information
  };

  // the lock map:
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  typedef typename ustl::map<T, SlotLock>::iterator map_it;
  ustl::map<T, SlotLock> active_locks_;
#else
  typedef typename std::map<T, SlotLock>::iterator map_it;
  std::map<T, SlotLock> active_locks_;
#endif

  // the mutex that protects the above map
#ifndef NO_USE_OF_MULTITHREADING
  mutable Mutex lock_map_lock_;
#endif
};

template<class T, typename U>
SlotLockManager<T,U>::SlotLockManager()
#ifndef NO_USE_OF_MULTITHREADING
: lock_map_lock_("SlotLockManager - ustl::map lock")
#endif
{
}

template<class T, typename U>
bool SlotLockManager<T,U>::setSlotAdditionalInfo(const T& slot, U info)
{
#ifndef NO_USE_OF_MULTITHREADING
  MutexLock mutex_lock(lock_map_lock_);
#endif

  for(map_it it = active_locks_.begin(); it != active_locks_.end(); it++)
  {
    if((*it).first == slot)
    {
      (*it).second.additional_info = info;
      return true;
    }
  }

  return false;
}

template<class T, typename U>
U SlotLockManager<T,U>::getSlotAdditionalInfo(const T& slot) const
{
#ifndef NO_USE_OF_MULTITHREADING
  MutexLock mutex_lock(lock_map_lock_);
#endif

  typename STL_NAMESPACE_PREFIX map<T, SlotLock>::const_iterator it;

  for(it = active_locks_.begin(); it != active_locks_.end(); it++)
  {
    if((*it).first == slot)
    {
      return (*it).second.additional_info;
    }
  }
  return 0;
}

#endif /* SLOTLOCKMANAGER_H_ */
