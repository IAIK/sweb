/**
 * Filename: TaskCopyExecutables.h
 * Description:
 *
 * Created on: 05.09.2012
 * Author: chris
 */

#ifndef TASKCOPYEXECUTABLES_H_
#define TASKCOPYEXECUTABLES_H_

#include "UtilTask.h"

class FsWorkingDirectory;

class TaskCopyFiles : public UtilTask
{
public:
  TaskCopyFiles(Program& image_util);
  virtual ~TaskCopyFiles();

  /**
   * executes the taks
   */
  virtual void execute(void);

  /**
   * returns the char identifying this option (e.g. h for help)
   */
  virtual char getOptionName(void) const;

  virtual const char* getDescription(void) const;

private:

  /**
   * copies a file from the OS's file system to the current
   * loaded image-file
   *
   * @param src
   * @param dest
   * @return true / false
   */
  bool copyFile(VfsSyscall* vfs, FsWorkingDirectory* wd_info, const char* src, const char* dest);

};

#endif /* TASKCOPYEXECUTABLES_H_ */
