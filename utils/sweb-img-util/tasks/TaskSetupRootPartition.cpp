/**
 * Filename: TaskSetupRootPartition.cpp
 * Description:
 *
 * Created on: 01.09.2012
 * Author: chris
 */

#include <iostream>
#include "TaskSetupRootPartition.h"

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
#define USE_FILE_SYSTEM_ON_GUEST_OS             1
#endif

#ifndef NO_USE_OF_MULTITHREADING
#define NO_USE_OF_MULTITHREADING                1
#endif

#include "fs/VfsSyscall.h"
#include "Program.h"
#include "fs/FsWorkingDirectory.h"

TaskSetupRootPartition::TaskSetupRootPartition(Program& image_util) : UtilTask(image_util)
{
}

TaskSetupRootPartition::~TaskSetupRootPartition()
{
}

void TaskSetupRootPartition::execute(void)
{
  std::cout << "setup ... setting up root partition" << std::endl;

  // std::cout << "" << std::endl;

  VfsSyscall* vfs = getVfsSyscallInstance( image_util_.getCurrentUsedPartition() );
  std::cout << "install on partition " << image_util_.getCurrentUsedPartition() << std::endl;

  if(vfs == NULL)
  {
    std::cout << "ERROR failed to load the given partition" << std::endl;
    return;
  }

  FsWorkingDirectory dir;

  // just create some folders on it
  vfs->mkdir(&dir, "/bin", 0755);
  vfs->mkdir(&dir, "/boot", 0755);
  vfs->mkdir(&dir, "/dev", 0755);
  vfs->mkdir(&dir, "/mnt", 0755);
  vfs->mkdir(&dir, "/usr", 0755);

  vfs->mkdir(&dir, "/usr/bin", 0755);
  vfs->mkdir(&dir, "/usr/lib", 0755);
  vfs->mkdir(&dir, "/usr/include", 0755);

  delete vfs;

  std::cout << "setup ... done with setting up the partition" << std::endl;
}

char TaskSetupRootPartition::getOptionName(void) const
{
  return 's';
}

const char* TaskSetupRootPartition::getDescription(void) const
{
  return "creates a typical linux setup on the given partition. (/bin /dev /mnt /usr ...)";
}
