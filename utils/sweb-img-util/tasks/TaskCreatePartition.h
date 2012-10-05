/**
 * Filename: TaskCreatePartition.h
 * Description:
 *
 * Created on: 13.09.2012
 * Author: chris
 */

#ifndef TASKCREATEPARTITION_H_
#define TASKCREATEPARTITION_H_

#include "UtilTask.h"

class TaskCreatePartition : public UtilTask
{
public:
  TaskCreatePartition(Program& image_util);
  virtual ~TaskCreatePartition();

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

#endif /* TASKCREATEPARTITION_H_ */
