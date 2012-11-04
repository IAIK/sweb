/**
 * Filename: ImageUtil.cpp
 * Description:
 *
 * Created on: 01.09.2012
 * Author: chris
 */

#include <stdint.h>

#include "Program.h"

#include "Util.h"

#include "UtilTask.h"
#include "TaskPrintHelp.h"
#include "TaskListPartitions.h"
#include "TaskFormatPartition.h"
#include "TaskSetupRootPartition.h"
#include "TaskCopyFiles.h"
#include "TaskInstallOnFlashDrive.h"

//#include "TaskTestFs.h"

Program::Program(std::vector<std::string>& args) : args_(args), image_info_(NULL),
    cur_partition_(1)
{
  createTasks();

  init();
}

Program::~Program()
{
  deleteTasks();

  if(image_info_ != NULL)
  {
    delete image_info_;
    image_info_ = NULL;
  }
}

void Program::init(void)
{
  if(args_.size() >= 4)
  {
    // reading arg nr 4
    Util::strToType(getArg(3), cur_partition_);
  }
}

bool Program::run(void)
{
  // arg format is as follows
  // 0 ... tool-name
  // 1 ... options
  // 2 ... image-filename
  //

  if(args_.size() < 2)
  {
    std::cout << "call with -h for details and help!" << std::endl;
    return false;
  }

  if(args_.size() >= 3)
  {
    setImageInfo( new ImageInfo(args_[2].c_str()) );
  }

  // parse tool options and perform them
  std::string& options = args_[1];

  if( !(options.length() > 1 && options[0] == '-') )
  {
    return false;
  }

  for(uint32_t i = 1; i < options.length(); i++)
  {
    // execute task
    UtilTask* task = findTask( options[i] );
    if(task == NULL)
    {
      std::cout << "error - unknown option -" << options[i] << "!" << std::endl;
    }
    else task->execute();
  }

  return true;
}

void Program::createTasks(void)
{
  tasks_.push_back( new TaskPrintHelp(*this) );
  tasks_.push_back( new TaskListPartitions(*this) );
  tasks_.push_back( new TaskFormatPartition(*this) );
  tasks_.push_back( new TaskSetupRootPartition(*this) );
  tasks_.push_back( new TaskCopyFiles(*this) );
  tasks_.push_back( new TaskInstallOnFlashDrive(*this) );

  // TODO add here more tasks
}

void Program::deleteTasks(void)
{
  for(uint32_t i = 0; i < tasks_.size(); i++)
  {
    delete tasks_[i];
  }
  tasks_.clear();
}

UtilTask* Program::findTask(char option_name)
{
  for(uint32_t i = 0; i < tasks_.size(); i++)
  {
    if(tasks_[i]->getOptionName() == option_name)
      return tasks_[i];
  }

  return NULL;
}

void Program::printTaskInfo(void) const
{
  for(uint32_t i = 0; i < tasks_.size(); i++)
  {
    std::cout << "-" << tasks_[i]->getOptionName() << " " << tasks_[i]->getDescription() << std::endl;
  }
}

ImageInfo* Program::getImageInfo(void)
{
  return image_info_;
}

void Program::setImageInfo(ImageInfo* info)
{
  if(image_info_ != NULL)
  {
    delete image_info_;
    image_info_ = NULL;
  }

  image_info_ = info;
}

int Program::getCurrentUsedPartition(void) const
{
  return cur_partition_;
}

unsigned int Program::getNumArgs(void) const
{
  return args_.size();
}

std::string Program::getArg(unsigned int arg_nr) const
{
  if(arg_nr >= args_.size())
    return std::string("");

  return args_[arg_nr];
}
