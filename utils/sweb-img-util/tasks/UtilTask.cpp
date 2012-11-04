/**
 * Filename: UtilTask.cpp
 * Description:
 *
 * Created on: 01.09.2012
 * Author: chris
 */

#include "UtilTask.h"
#include "fs/VfsSyscall.h"
#include "fs/device/FsDeviceFile.h"
#include "Program.h"
#include "PartitionInfo.h"
#include "ImageInfo.h"
#include <iostream>
#include <assert.h>

UtilTask::UtilTask(Program& image_util) : image_util_(image_util)
{
}

UtilTask::~UtilTask()
{
}

const char* UtilTask::getDescription(void) const
{
  return "< empty >";
}

FsDevice* UtilTask::getNewDeviceInstance(uint32_t partition)
{
  ImageInfo* img_info = image_util_.getImageInfo();
  if(img_info == NULL) return NULL;

  const PartitionInfo* part_info = img_info->getPartition(partition);
  if(part_info == NULL) return NULL;

  return new FsDeviceFile(img_info->getFilename(),
      part_info->getPartitionSectorOffset() * part_info->getSectorSize(),
      part_info->getNumSectors() * part_info->getSectorSize());
}

VfsSyscall* UtilTask::getVfsSyscallInstance(uint32_t partition)
{
  // obtaining hte ImageInfo object
  ImageInfo* img_info = image_util_.getImageInfo();

  if(partition >= img_info->getNumPartitions())
  {
    std::cout << "sorry, current image does not have " << partition << " partitions!" << std::endl;
    return NULL;
  }

  // getting the partition infos
  const PartitionInfo* part_info = img_info->getPartition(partition);
  assert(part_info != NULL);

  //std::cout << "partition is " << *part_info << std::endl;

  // create the Device, simulate a Volume driver by just reading with
  // the default syscalls from the image-file
  FsDevice* dev = getNewDeviceInstance(partition);
  /*FsDevice* dev = new FsDeviceFile(img_info->getFilename(),
                                   part_info->getPartitionSectorOffset() * part_info->getSectorSize(),
                                   part_info->getNumSectors() * part_info->getSectorSize());*/

  // create a Vfs-Syscall instance
  return new VfsSyscall(dev, part_info->getPartitionIdentfier());
}
