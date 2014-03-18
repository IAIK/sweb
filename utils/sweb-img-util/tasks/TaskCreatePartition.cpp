/**
 * Filename: TaskCreatePartition.cpp
 * Description:
 *
 * Created on: 13.09.2012
 * Author: chris
 */

#include "TaskCreatePartition.h"

TaskCreatePartition::TaskCreatePartition(Program& image_util) : UtilTask(image_util)
{
}

TaskCreatePartition::~TaskCreatePartition()
{
}

void TaskCreatePartition::execute(void)
{
  // TODO implement!!
}

char TaskCreatePartition::getOptionName(void) const
{
  return 'p';
}

const char* TaskCreatePartition::getDescription(void) const
{
  return "creates a new partition with the given size; if size is bigger than the space available or 0 size will be choosen to fit exactly with the available space";
}
