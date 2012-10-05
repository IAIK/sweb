/**
 * Filename: FsLockSimple.cpp
 * Description:
 *
 * Created on: 13.05.2012
 * Author: chris
 */

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS

#include "fs/FsLockSimple.h"

FsLockSimple::FsLockSimple() : fs_lock_("FsLockSimple")
{
}

FsLockSimple::~FsLockSimple()
{
}

bool FsLockSimple::acquireReadNonBlocking(void)
{
  return fs_lock_.acquireNonBlocking("FsLockSimple::acquireReadNonBlocking");
}

void FsLockSimple::acquireReadBlocking(void)
{
  fs_lock_.acquire("FsLockSimple::acquireReadBlocking");
}

void FsLockSimple::releaseRead(void)
{
  fs_lock_.release("FsLockSimple::releaseRead");
}

bool FsLockSimple::acquireWriteNonBlocking(void)
{
  return fs_lock_.acquireNonBlocking("FsLockSimple::acquireWriteNonBlocking");
}

void FsLockSimple::acquireWriteBlocking(void)
{
  fs_lock_.acquire("FsLockSimple::acquireWriteBlocking");
}

void FsLockSimple::releaseWrite(void)
{
  fs_lock_.release("FsLockSimple::releaseWrite");
}

#endif // USE_FILE_SYSTEM_ON_GUEST_OS
