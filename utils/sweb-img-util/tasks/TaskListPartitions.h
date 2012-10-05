/**
 * Filename: TaskListPartitions.h
 * Description:
 *
 * Created on: 01.09.2012
 * Author: chris
 */

#ifndef TASKLISTPARTITIONS_H_
#define TASKLISTPARTITIONS_H_

#include "UtilTask.h"

class TaskListPartitions : public UtilTask
{
public:
  TaskListPartitions(Program& image_util);
  virtual ~TaskListPartitions();

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

#endif /* TASKLISTPARTITIONS_H_ */
