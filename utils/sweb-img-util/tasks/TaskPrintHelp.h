/**
 * Filename: TaskPrintHelp.h
 * Description:
 *
 * Created on: 01.09.2012
 * Author: chris
 */

#ifndef TASKPRINTHELP_H_
#define TASKPRINTHELP_H_

#include "UtilTask.h"

class TaskPrintHelp : public UtilTask
{
public:
  TaskPrintHelp(Program& image_util);

  virtual ~TaskPrintHelp();

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

#endif /* TASKPRINTHELP_H_ */
