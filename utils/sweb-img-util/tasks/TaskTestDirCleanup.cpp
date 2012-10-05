/**
 * Filename: TaskTestDirCleanup.cpp
 * Description:
 *
 * Created on: 03.09.2012
 * Author: chris
 */

#include "TaskTestDirCleanup.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "fs/VfsSyscall.h"
#include "fs/FsWorkingDirectory.h"
#include "fs/Statfs.h"

#include "Thread.h"

TaskTestDirCleanup::TaskTestDirCleanup(Program& image_util) : UtilTask(image_util),
    num_directories_to_create_(650)
{
}

TaskTestDirCleanup::~TaskTestDirCleanup()
{
}

void TaskTestDirCleanup::execute(void)
{
  debug(FS_TESTCASE, "---- FsTCDirectoryCleanup ... ----\n");

  // getting a Vfs-Syscall instance from partition 1
  VfsSyscall* vfs = getVfsSyscallInstance( image_util_.getCurrentUsedPartition() );

  if(vfs == NULL)
  {
    std::cout << "ERROR failed to mount-image partition" << std::endl;
    return;
  }

  // just a dummy thread
  Thread thread(new FsWorkingDirectory());

  // 1. create new tmp directory
  vfs->mkdir(&thread, "tmp", 0755);
  vfs->chdir(&thread, "tmp");

  // 2. save stats before executing the test-case
  statfs_s* init_stat = vfs->statfs(&thread, "./");

  // 3. create 1000 sub-directories (which are empty)
  char dir_name[30];

  for(uint32 i = 0; i < num_directories_to_create_; i++)
  {
    sprintf(dir_name, "%d", i+1);
    //itoa(i+1, dir_name, 10);

    debug(FS_TESTCASE, "creating sub-dir \"%s\"\n", dir_name);

    if( vfs->mkdir(&thread, dir_name, 0755) < 0 )
    {
      debug(FS_TESTCASE, "ERROR failed to create \"%s\"!\n", dir_name);
    }
  }

  // 3.1 save stats
  statfs_s* mkdir_stats = vfs->statfs(&thread, "./");

  // 4. rm all 100 sub-directories
  for(uint32 i = 0; i < num_directories_to_create_; i++)
  {
    sprintf(dir_name, "%d", i+1);
    //itoa(i+1, dir_name, 10);
    debug(FS_TESTCASE, "removing sub-dir \"%s\"\n", dir_name);

    if ( vfs->rmdir(&thread, dir_name) < 0 )
    {
      debug(FS_TESTCASE, "ERROR failed to delete \"%s\"!\n", dir_name);
    }
  }

  // 5. cmp final stats to initial stats (should be equal!)
  statfs_s* final_stat = vfs->statfs(&thread, "./");

  // printing out the stats
  debug(FS_TESTCASE, "Init\tmkdir\tFinal\t\n");
  debug(FS_TESTCASE, "%d\t%d\t%d\tblocks\n", init_stat->num_blocks, mkdir_stats->num_blocks, final_stat->num_blocks);
  debug(FS_TESTCASE, "%d\t%d\t%d\tfree blocks\n", init_stat->num_free_blocks, mkdir_stats->num_free_blocks, final_stat->num_free_blocks);
  debug(FS_TESTCASE, "%d\t%d\t%d\tinodes\n", init_stat->max_inodes, mkdir_stats->max_inodes, final_stat->max_inodes);
  debug(FS_TESTCASE, "%d\t%d\t%d\tused-nodes\n\n", init_stat->used_inodes, mkdir_stats->used_inodes, final_stat->used_inodes);

  debug(FS_TESTCASE, "I-Node cache statistics\n");
  debug(FS_TESTCASE, "requests %d - hits %d - misses %d - evicts %d\n", init_stat->inode_cache_stat.num_requests,
      init_stat->inode_cache_stat.num_cache_hits, init_stat->inode_cache_stat.num_misses, init_stat->inode_cache_stat.evicted_items);

  if(init_stat->num_free_blocks != final_stat->num_free_blocks
      || init_stat->used_inodes != final_stat->used_inodes)
  {
    debug(FS_TESTCASE, "---- FsTCDirectoryCleanup [FAIL] ----\n");
  }
  else
  {
    debug(FS_TESTCASE, "---- FsTCDirectoryCleanup [DONE] ----\n");
  }

  delete init_stat;
  delete mkdir_stats;
  delete final_stat;

  delete vfs;
}

char TaskTestDirCleanup::getOptionName(void) const
{
  return 'd';
}
