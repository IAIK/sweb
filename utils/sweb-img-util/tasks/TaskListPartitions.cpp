/**
 * Filename: TaskListPartitions.cpp
 * Description:
 *
 * Created on: 01.09.2012
 * Author: chris
 */

#include "TaskListPartitions.h"

TaskListPartitions::TaskListPartitions(Program& image_util) : UtilTask(image_util)
{
}

TaskListPartitions::~TaskListPartitions()
{
}

void TaskListPartitions::execute(void)
{
  ImageInfo* img_info = image_util_.getImageInfo();

  if(img_info == NULL)
  {
    std::cout << "-l error - no image file specified!" << std::endl;
    return;
  }

  // printing the ImageInfo
  std::cout << (*img_info) << std::endl;
}

char TaskListPartitions::getOptionName(void) const
{
  return 'l';
}

const char* TaskListPartitions::getDescription(void) const
{
  return "lists all partitions of the current image and displays a few additional info";
}
