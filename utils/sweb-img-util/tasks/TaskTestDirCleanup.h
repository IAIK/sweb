/**
 * Filename: TaskTestDirCleanup.h
 * Description:
 *
 * Created on: 03.09.2012
 * Author: chris
 */

#ifndef TASKTESTDIRCLEANUP_H_
#define TASKTESTDIRCLEANUP_H_

#include "UtilTask.h"

class TaskTestDirCleanup : public UtilTask
{
public:
  TaskTestDirCleanup(Program& image_util);

  virtual ~TaskTestDirCleanup();

  /**
   * executes the taks
   */
  virtual void execute(void);

  /**
   * returns the char identifying this option (e.g. h for help)
   */
  virtual char getOptionName(void) const;

private:

  // num directories to create
  uint32 num_directories_to_create_;

};

#endif /* TASKTESTDIRCLEANUP_H_ */
