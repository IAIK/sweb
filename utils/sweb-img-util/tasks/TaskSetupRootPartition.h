/**
 * Filename: TaskSetupRootPartition.h
 * Description:
 *
 * Created on: 01.09.2012
 * Author: chris
 */

#ifndef TASKSETUPROOTPARTITION_H_
#define TASKSETUPROOTPARTITION_H_

#include "UtilTask.h"

/**
 * @class
 */
class TaskSetupRootPartition : public UtilTask
{
public:
  /**
   * constructor
   */
  TaskSetupRootPartition(Program& image_util);

  /**
   * destructor
   */
  virtual ~TaskSetupRootPartition();

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

#endif /* TASKSETUPROOTPARTITION_H_ */
