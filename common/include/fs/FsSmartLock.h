/**
 * Filename: FsSmartLock.h
 * Description:
 *
 * Created on: 16.05.2012
 * Author: chris
 */

#ifndef FSSMARTLOCK_H_
#define FSSMARTLOCK_H_

// forward
class FileSystemLock;

/**
 * a smart FileSystem lock, that frees it's acquired resource when
 * this object is destroyed
 */
class FsSmartLock
{
  public:
    FsSmartLock(FileSystemLock* fs_lock);
    virtual ~FsSmartLock();

    /**
     * performs a manual release of the FileSystemLock
     */
    virtual void doManualRelease(void) = 0;

    enum AutoAcquire { ACQUIRE_LOCK, LOCK_IS_ALREADY_ACQUIRED };

  protected:

    // the managed lock
    FileSystemLock* fs_lock_;

    // is there a need to release the resource?
    bool relase_on_destruction_;
};

/**
 * a special read-auto-lock that releases the Lock on destruction
 */
class FsReadSmartLock : public FsSmartLock
{
  public:
    FsReadSmartLock(FileSystemLock* fs_lock, AutoAcquire acquire = ACQUIRE_LOCK);
    virtual ~FsReadSmartLock();

    /**
     * performs a manual release of the FileSystemLock
     */
    virtual void doManualRelease(void);
};

/**
 * a special read-auto-lock that releases the Lock on destruction
 */
class FsWriteSmartLock : public FsSmartLock
{
  public:
    FsWriteSmartLock(FileSystemLock* fs_lock, AutoAcquire acquire = ACQUIRE_LOCK);
    virtual ~FsWriteSmartLock();

    /**
     * performs a manual release of the FileSystemLock
     */
    virtual void doManualRelease(void);
};

#endif /* FSSMARTLOCK_H_ */
