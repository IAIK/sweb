/**
 * Filename: FsLockDummy.h
 * Description:
 *
 * Created on: 02.09.2012
 * Author: chris
 */

#ifndef FSLOCKDUMMY_H_
#define FSLOCKDUMMY_H_

#include "FileSystemLock.h"

/**
 * @class the Dummy Lock does not do anything or establishes a lock
 * mechanism, it is just used under a host OS were multithreading
 * is not used (e.g. the sweb-img-util)
 */
class FsLockDummy : public FileSystemLock
{
public:
  FsLockDummy();
  virtual ~FsLockDummy();

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

};

#endif /* FSLOCKDUMMY_H_ */
