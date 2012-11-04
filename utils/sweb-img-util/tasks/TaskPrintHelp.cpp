/**
 * Filename: TaskPrintHelp.cpp
 * Description:
 *
 * Created on: 01.09.2012
 * Author: chris
 */

#include <iostream>
#include "TaskPrintHelp.h"
#include "Program.h"

TaskPrintHelp::TaskPrintHelp(Program& image_util) : UtilTask(image_util)
{
}

TaskPrintHelp::~TaskPrintHelp()
{
}

void TaskPrintHelp::execute(void)
{
  // print some nice little help text
  std::cout << "sweb-img-util SWEB's image utility tool" << std::endl;
  std::cout << "---------------------------------------" << std::endl;

  std::cout << std::endl << "available options:" << std::endl;

  // prints all currently available tasks and their info-text
  image_util_.printTaskInfo();

  // std::cout << "created by Christopher Walles" << std::endl;
}

char TaskPrintHelp::getOptionName(void) const
{
  return 'h';
}

const char* TaskPrintHelp::getDescription(void) const
{
  return "prints this help text";
}
