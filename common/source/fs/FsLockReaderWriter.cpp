/**
 * Filename: FsLockReaderWriter.cpp
 * Description:
 *
 * Created on: 13.05.2012
 * Author: chris
 */

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS

#include "fs/FsLockReaderWriter.h"

#include "kernel/Scheduler.h"

FsLockReaderWriter::FsLockReaderWriter() : FileSystemLock(), num_readers_(0),
    num_write_requests_(0), writer_active_(false), lock_("FsLockReaderWriter")
{
}

FsLockReaderWriter::~FsLockReaderWriter()
{
}

bool FsLockReaderWriter::acquireReadNonBlocking(void)
{
  MutexLock auto_lock(lock_);

  // no write request here, allow to enter another reader
  if(!writer_active_ && num_write_requests_ == 0)
  {
    num_readers_++;
    return true;
  }

  // there is a write-request / or writer in the critical section
  return false;
}

void FsLockReaderWriter::acquireReadBlocking(void)
{
  while(true)
  {
    lock_.acquire("acquireReadBlocking - lock acquire");

    if(!writer_active_ && num_write_requests_ == 0)
    {
      num_readers_++;
      lock_.release("acquireReadBlocking - lock release");
      break;
    }

    lock_.release("acquireReadBlocking - lock release");
    Scheduler::instance()->yield();
  }
}

void FsLockReaderWriter::releaseRead(void)
{
  lock_.acquire("releaseRead - lock acquire");
  num_readers_--;
  lock_.release("releaseRead - lock release");
}

bool FsLockReaderWriter::acquireWriteNonBlocking(void)
{
  bool ret = false;

  lock_.acquire("acquireWriteNonBlocking - lock acquire");

  // no readers AND no writer here
  if(num_readers_ == 0 && writer_active_ == false)
  {
    // we have the lock!
    writer_active_ = true;
    ret = true;
  }
  lock_.release("acquireWriteNonBlocking - lock release");

  return ret;
}

void FsLockReaderWriter::acquireWriteBlocking(void)
{
  // request exclusive access!
  lock_.acquire("acquireWriteBlocking - lock acquire");
  num_write_requests_++;
  lock_.release("acquireWriteBlocking - lock release");

  // waiting for the lock
  while(true)
  {
    lock_.acquire("acquireWriteBlocking - lock acquire");

    bool access_gained = false;

    // lock has became available
    if(writer_active_ == false && num_readers_ == 0)
    {
      // we are now the write; one request less
      writer_active_ = true;
      num_write_requests_--;
      access_gained = true;
    }
    lock_.release("acquireWriteBlocking - lock release");

    if(access_gained == true)
      break;

    // next one to go ...
    Scheduler::instance()->yield();
  }
}

void FsLockReaderWriter::releaseWrite(void)
{
  lock_.acquire("releaseWrite - lock acquire");
  writer_active_ = false;
  lock_.release("releaseWrite - lock release");
}

#endif // USE_FILE_SYSTEM_ON_GUEST_OS
