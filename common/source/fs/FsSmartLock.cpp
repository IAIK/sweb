/**
 * Filename: FsSmartLock.cpp
 * Description:
 *
 * Created on: 16.05.2012
 * Author: chris
 */

#include "fs/FsSmartLock.h"
#include "fs/FileSystemLock.h"

FsSmartLock::FsSmartLock(FileSystemLock* fs_lock) : fs_lock_(fs_lock), relase_on_destruction_(false)
{
}

FsSmartLock::~FsSmartLock()
{
}

FsReadSmartLock::FsReadSmartLock(FileSystemLock* fs_lock, AutoAcquire acquire) :
    FsSmartLock(fs_lock)
{
  if(acquire == ACQUIRE_LOCK)
  {
    fs_lock_->acquireReadBlocking();
  }

  relase_on_destruction_ = true;
}

FsReadSmartLock::~FsReadSmartLock()
{
  if(relase_on_destruction_)
  {
    fs_lock_->releaseRead();
  }
}

void FsReadSmartLock::doManualRelease(void)
{
  fs_lock_->releaseRead();
  relase_on_destruction_ = false;
}

FsWriteSmartLock::FsWriteSmartLock(FileSystemLock* fs_lock, AutoAcquire acquire) :
    FsSmartLock(fs_lock)
{
  if(acquire == ACQUIRE_LOCK)
  {
    fs_lock_->acquireWriteBlocking();
  }

  relase_on_destruction_ = true;
}

FsWriteSmartLock::~FsWriteSmartLock()
{
  if(relase_on_destruction_)
  {
    fs_lock_->releaseWrite();
  }
}

void FsWriteSmartLock::doManualRelease(void)
{
  fs_lock_->releaseWrite();
  relase_on_destruction_ = false;
}
