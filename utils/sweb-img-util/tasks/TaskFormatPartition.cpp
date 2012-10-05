/**
 * Filename: TaskFormatPartition.cpp
 * Description:
 *
 * Created on: 05.09.2012
 * Author: chris
 */

#include "TaskFormatPartition.h"
#include "Util.h"

#include "fs/device/FsDevice.h"
#include "fs/device/FsDeviceFile.h"

#include "fs/FileSystem.h"
#include "fs/minix/FileSystemMinix.h"
#include "fs/minix/FormatMinixPartition.h"

TaskFormatPartition::TaskFormatPartition(Program& image_util) : UtilTask(image_util)
{
}

TaskFormatPartition::~TaskFormatPartition()
{
}

void TaskFormatPartition::execute(void)
{
  ImageInfo* img_info = image_util_.getImageInfo();

  if(img_info == NULL)
  {
    std::cout << "-f error - no image file specified!" << std::endl;
    return;
  }

  int32 cur_partition = image_util_.getCurrentUsedPartition();

  if(cur_partition < 0 || cur_partition >= img_info->getNumPartitions())
  {
    std::cout << "ERROR unknown partition - can not install new filesystem to partition " << cur_partition << std::endl;
    return;
  }

  // consider the 4th parameter
  std::string fs_name = image_util_.getArg(ARG_FS_NAME);

  if(fs_name == "")
  {
    std::cout << "-f error - no file-system name specified!" << std::endl;
    return;
  }

  std::cout << "going to format partition " << cur_partition << " with " << fs_name << std::endl;

  // get a new FsDevice instance
  FsDevice* device = getNewDeviceInstance(cur_partition);

  // zone / cluster size for the new FS in bytes
  sector_addr_t zone_size = getZoneSize();
  int32 fs_version_to_create = getVersion();

  // TODO very, very dirty, shall be solved in a much cleaner way!!
  if( fs_name == "minix" || fs_name == "minixfs" )
  {
    createMinixPartition(device);
  }
  else
  {
    std::cout << "-f error - unknown file-system " << fs_name << std::endl;
  }

  delete device;
}

char TaskFormatPartition::getOptionName(void) const
{
  return 'f';
}

sector_addr_t TaskFormatPartition::getZoneSize(void)
{
  sector_addr_t zone_size = 1024;
  Util::strToType(image_util_.getArg(ARG_ZONE_SIZE), zone_size);
  if(zone_size % 512 != 0) zone_size = 1024;

  return zone_size;
}

int32 TaskFormatPartition::getVersion(void)
{
  int32 version_to_install = 1;
  Util::strToType(image_util_.getArg(ARG_FS_VERSION), version_to_install);

  return version_to_install;
}

void TaskFormatPartition::createMinixPartition(FsDevice* device)
{
  // determine the used filename length for the new Minix
  sector_addr_t zone_size = getZoneSize();
  int32 version = getVersion();
  int32 filename_len = 30; // by default filenames are 30chars long

  // read user-input values
  Util::strToType(image_util_.getArg(ARG_OTHER_1), filename_len);

  if(filename_len != 14 && filename_len != 30)
    filename_len = 30;

  std::cout << "create a new minix Version=" << version << " zone-size " << zone_size << " filename len " << filename_len << std::endl;

  // format minix on the Device
  if(FormatMinixPartition::format( device, getZoneSize(), getVersion(), filename_len ))
  {
    std::cout << "created new Minix partition" << std::endl;
  }
  else
  {
    std::cout << "ERROR - failed to created new Minix partition" << std::endl;
  }
}

const char* TaskFormatPartition::getDescription(void) const
{
  return "formats the given partition with the given FileSystem - WARNING all data will be lost!\n   Arguments : img-file <partition-no> <fs-name> <zone/cluster-size> <version> <filename-length>";
}
