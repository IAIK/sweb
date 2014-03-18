/**
 * Filename: FsLockReaderWriter.h
 * Description:
 *
 * Created on: 13.05.2012
 * Author: chris
 */

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS

#ifndef FSLOCKREADERWRITER_H_
#define FSLOCKREADERWRITER_H_

#include "FileSystemLock.h"

#include "types.h"
#include "kernel/Mutex.h"

/**
 * @class a Reader / Writers lock-system
 */
class FsLockReaderWriter : public FileSystemLock
{
  public:
    FsLockReaderWriter();
    virtual ~FsLockReaderWriter();

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

    // number of readers
    uint32 num_readers_;

    // a request to write data is here
    uint32 num_write_requests_;
    bool writer_active_;

    // lock
    Mutex lock_;
};

#endif /* FSLOCKREADERWRITER_H_ */

#endif // USE_FILE_SYSTEM_ON_GUEST_OS
