/**
 * Filename: FileSystemLock.h
 * Description:
 *
 * Created on: 13.05.2012
 * Author: chris
 */

#ifndef FILESYSTEMLOCK_H_
#define FILESYSTEMLOCK_H_

/**
 * @class FileSystemLock "interface" - for more sophisticated lock-mechanisms
 */
class FileSystemLock
{
  public:
    FileSystemLock();
    virtual ~FileSystemLock();

    /**
     * acquires / releases the lock for reading data
     */
    virtual bool acquireReadNonBlocking(void) = 0;
    virtual void acquireReadBlocking(void) = 0;
    virtual void releaseRead(void) = 0;

    /**
     * acquires / releases the lock for writing data
     */
    virtual bool acquireWriteNonBlocking(void) = 0;
    virtual void acquireWriteBlocking(void) = 0;
    virtual void releaseWrite(void) = 0;

    /**
     * returns the most sophisticated and efficient lock-implementation for
     * file-system locks
     * TODO change this method!
     */
    static FileSystemLock* getNewFSLock(void);
};

#endif /* FILESYSTEMLOCK_H_ */
