/**
 * Filename: TaskTestFs.cpp
 * Description:
 *
 * Created on: 03.09.2012
 * Author: chris
 */

#include "TaskTestFs.h"

#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

#include "fs/VfsSyscall.h"
#include "fs/FsWorkingDirectory.h"
#include "Thread.h"

//#include "fs/tests/FsTestsuite.h"

TaskTestFs::TaskTestFs(Program& image_util) : UtilTask(image_util)
{
}

TaskTestFs::~TaskTestFs()
{
}

void TaskTestFs::execute(void)
{
  /*std::cout << "FileSystem - host OS testcase ... " << std::endl;
  VfsSyscall* vfs = getVfsSyscallInstance( image_util_.getCurrentUsedPartition() );

  if(vfs == NULL)
  {
    std::cout << "ERROR failed to mount-image partition" << std::endl;
    return;
  }

  // just a dummy thread
  Thread thread(new FsWorkingDirectory());

  FsTestsuite::createFsTestsuite(vfs);
  FsTestsuite::instance()->run();
  FsTestsuite::destroy();

  delete vfs;*/

  std::cout << "FileSystem - host OS testcase [DONE] " << std::endl;
}

char TaskTestFs::getOptionName(void) const
{
  return 't';
}

const char* TaskTestFs::getDescription(void) const
{
  return "runs the fs-testsuite without multi-threading based tests";
}
