/**
 * Filename: TaskTestFs.h
 * Description:
 *
 * Created on: 03.09.2012
 * Author: chris
 */

#ifndef TASKTESTFS_H_
#define TASKTESTFS_H_

#include "UtilTask.h"

class TaskTestFs : public UtilTask
{
public:
  /**
   * constructor
   *
   * @param image_util
   */
  TaskTestFs(Program& image_util);

  /**
   *
   */
  virtual ~TaskTestFs();

  /**
   * executes the taks
   */
  virtual void execute(void);

  /**
   * returns the char identifying this option (e.g. h for help)
   */
  virtual char getOptionName(void) const;

  virtual const char* getDescription(void) const;

};

#endif /* TASKTESTFS_H_ */
