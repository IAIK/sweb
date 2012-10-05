/**
 * Filename: FileSystemLock.cpp
 * Description:
 *
 * Created on: 13.05.2012
 * Author: chris
 */

#include "fs/FileSystemLock.h"

#include "fs/FsLockDummy.h"
//#include "fs/FsLockSimple.h"
#include "fs/FsLockReaderWriter.h"
// TODO your highly sophisticated FS-locking method-here!

FileSystemLock::FileSystemLock()
{
}

FileSystemLock::~FileSystemLock()
{
}

FileSystemLock* FileSystemLock::getNewFSLock(void)
{
#ifdef USE_FILE_SYSTEM_ON_GUEST_OS
  return new FsLockDummy();
#else
  return new FsLockReaderWriter(); // FsLockSimple();
#endif
}
