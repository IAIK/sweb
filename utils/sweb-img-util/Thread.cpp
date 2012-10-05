/**
 * Filename: Thread.cpp
 * Description:
 *
 * Created on: 02.09.2012
 * Author: chris
 */

#include "Thread.h"

#include "fs/FsWorkingDirectory.h"

Thread::Thread() : working_dir_(0)
{
}

Thread::Thread(FsWorkingDirectory* wd, const char* name) : working_dir_(wd)
{
}

Thread::~Thread()
{
  if(working_dir_ != 0)
  {
    delete working_dir_;
  }
}

FsWorkingDirectory* Thread::getWorkingDirInfo(void)
{
  return working_dir_;
}

void Thread::setWorkingDirInfo(FsWorkingDirectory* working_dir)
{
  working_dir_ = working_dir;
}

void Thread::Run(void)
{
  // does nothing here
}
