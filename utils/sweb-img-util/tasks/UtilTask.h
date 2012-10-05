/**
 * Filename: UtilTask.h
 * Description:
 *
 * Created on: 01.09.2012
 * Author: chris
 */

#ifndef UTILTASK_H_
#define UTILTASK_H_

#include "Program.h"

#include "fs/VfsSyscall.h"

/**
 * @class
 */
class UtilTask
{
public:
  /**
   * constructor
   *
   * @param image_util the Program Manager class
   */
  UtilTask(Program& image_util);

  /**
   * destructor
   */
  virtual ~UtilTask();

  /**
   * executes the taks
   */
  virtual void execute(void) = 0;

  /**
   * returns the char identifying this option (e.g. h for help)
   */
  virtual char getOptionName(void) const = 0;

  /**
   * getting a description of the function
   * @return function description
   */
  virtual const char* getDescription(void) const;

  /**
   * get a new created Vfs-Syscall instance which mounted
   * the given partition of the current loaded image-file
   *
   * @param partition the number of the partition to mount
   * @return the VfsSyscall instance or NULL
   */
  VfsSyscall* getVfsSyscallInstance(uint32 partition);

  /**
   * get a new FsDevice instance for the current loaded image
   * and the given partition
   */
  FsDevice* getNewDeviceInstance(uint32 partition);

protected:

  // the utility class managing the program
  Program& image_util_;

};

#endif /* UTILTASK_H_ */
