/**
 * Filename: FsLockDummy.cpp
 * Description:
 *
 * Created on: 02.09.2012
 * Author: chris
 */

#include "fs/FsLockDummy.h"

FsLockDummy::FsLockDummy()
{
}

FsLockDummy::~FsLockDummy()
{
}

bool FsLockDummy::acquireReadNonBlocking(void)
{
  return true;
}

void FsLockDummy::acquireReadBlocking(void)
{
  // dummy does nothing... :D
}

void FsLockDummy::releaseRead(void)
{
  // dummy does nothing... :D
}

bool FsLockDummy::acquireWriteNonBlocking(void)
{
  // dummy does nothing... :D
  return true;
}

void FsLockDummy::acquireWriteBlocking(void)
{
  // dummy does nothing... :D
}

void FsLockDummy::releaseWrite(void)
{
  // dummy does nothing... :D
}
