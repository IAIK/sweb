/**
 * Filename: FsLockSimple.h
 * Description:
 *
 * Created on: 13.05.2012
 * Author: chris
 */

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS

#ifndef FSLOCKSIMPLE_H_
#define FSLOCKSIMPLE_H_

#include "Mutex.h"

#include "FileSystemLock.h"

/**
 * @class FsLockSimple very simple but inefficient Locking method for the
 * FileSystem
 */
class FsLockSimple : public FileSystemLock
{
  public:
    FsLockSimple();
    virtual ~FsLockSimple();

    /**
     * acquires / releases the lock for reading data
     */
    virtual bool acquireReadNonBlocking(void);
    virtual void acquireReadBlocking(void);
    virtual void releaseRead(void);

    /**
     * acquires / releases the lock for writing data
     */
    virtual bool acquireWriteNonBlocking(void);
    virtual void acquireWriteBlocking(void);
    virtual void releaseWrite(void);

  private:

    Mutex fs_lock_;
};

#endif /* FSLOCKSIMPLE_H_ */

#endif // USE_FILE_SYSTEM_ON_GUEST_OS
