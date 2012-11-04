/**
 * Filename: TaskCopyExecutables.cpp
 * Description:
 *
 * Created on: 05.09.2012
 * Author: chris
 */

#include "TaskCopyFiles.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "fs/VfsSyscall.h"
#include "fs/FsWorkingDirectory.h"
#include "fs/Statfs.h"

TaskCopyFiles::TaskCopyFiles(Program& image_util) : UtilTask(image_util)
{
}

TaskCopyFiles::~TaskCopyFiles()
{
}

void TaskCopyFiles::execute(void)
{
  int partition_to_copy_to = image_util_.getCurrentUsedPartition();

  // print some nice little help text
  std::cout << "copying given files to partition " << partition_to_copy_to << std::endl;

  // getting a Vfs-Syscall instance from partition 1
  VfsSyscall* vfs = getVfsSyscallInstance( partition_to_copy_to );

  // just a dummy thread
  FsWorkingDirectory wd_info;

  // call format of this task is as follows
  // sweb-img-util -x <img-file> <partition-number> [<file.0-src> <file.0-dest>]*

  // start to copy the files
  for(uint32 i = 4; i < image_util_.getNumArgs(); i+=2)
  {
    std::string src_filename = image_util_.getArg(i);
    std::string dst_filename = image_util_.getArg(i+1);

    std::cout << "copying " << src_filename << " (as \"" << dst_filename << "\") to the image-file" << std::endl;

    // copy the file to the image
    int32 retry = 3;
    while (retry > 0 && !copyFile(vfs, &wd_info, src_filename.c_str(), dst_filename.c_str()))
    {
      --retry;
    }

  }

  // delete VFS instance
  delete vfs;

  std::cout << "copying files was successful" << std::endl;
}

char TaskCopyFiles::getOptionName(void) const
{
  return 'x';
}

const char* TaskCopyFiles::getDescription(void) const
{
  return "copies all files specified to the given partition. call with : -x <img-file> <partition-no> [<file.0-src> <file.0-dest>]*";
}

bool TaskCopyFiles::copyFile(VfsSyscall* vfs, FsWorkingDirectory* wd_info, const char* src, const char* dest)
{
  // create the destination file
  int32 fd = vfs->creat(wd_info, dest);

  if(fd <= 0)
  {
    std::cout << "ERROR - failed to create \"" << dest << "\" on image!" << std::endl;
    return false;
  }

  // open the source-file
  int src_file = open(src, O_RDONLY);

  if(src_file <= 0)
  {
    std::cout << "ERROR - failed to open the source-file!" << std::endl;

    vfs->close(wd_info, fd);
    return false;
  }

  // determine the file-size of the image:
  loff_t file_size = lseek(src_file, 0, SEEK_END);
  lseek(src_file, 0, SEEK_SET);

  // copy the image's data
  loff_t bytes_copied = 0;
  char* chunk_buf = new char[1024];

  while(bytes_copied < file_size)
  {
    uint32 buf_len = 1024;

    if(bytes_copied + buf_len > file_size)
    {
      buf_len = file_size - bytes_copied;
    }

    assert( read(src_file, chunk_buf, buf_len) == buf_len );
    assert( vfs->write(wd_info, fd, chunk_buf, buf_len) == buf_len );
    bytes_copied += buf_len;
  }

  delete[] chunk_buf;

  // close it
  vfs->close(wd_info, fd);

  return true;
}

