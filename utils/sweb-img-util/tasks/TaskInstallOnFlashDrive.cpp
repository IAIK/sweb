/**
 * Filename: TaskCopyExecutables.cpp
 * Description:
 *
 * Created on: 05.09.2012
 * Author: chris
 */

#include "TaskInstallOnFlashDrive.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "fs/VfsSyscall.h"
#include "fs/FsWorkingDirectory.h"
#include "fs/Statfs.h"


TaskInstallOnFlashDrive::TaskInstallOnFlashDrive(Program& image_util) : UtilTask(image_util)
{
}

TaskInstallOnFlashDrive::~TaskInstallOnFlashDrive()
{
}

bool TaskInstallOnFlashDrive::promtToProceed(const char* message)
{
  std::cout << message << std::endl;
  std::cout << "are you sure you want to proceed? type \"yes\" and hit [RETURN] to proceed!" << std::endl;

  std::string input;
  std::cin >> input;

  if(input == "yes")
  {
    return true;
  }

  return false;
}

bool TaskInstallOnFlashDrive::printInitialWarning(void)
{
  std::cout << "------------------------------------------------------" << std::endl;
  std::cout << " SWEB-img-util : install SWEB to device" << std::endl;
  std::cout << "------------------------------------------------------" << std::endl;

  //std::cout << "" << std::endl;

  std::string init_warning = "WARNING: the usage of this feature can cause a irreversible loss of data!\n";
  init_warning += "THERE IS ABSOLUTLY NO WARRANTY GIVEN BY THE AUTHOR!\n";
  init_warning += "\n";
  init_warning += "BY continuing YOU AGREE THAT YOU USE THIS FEATURE ON YOUR OWN RISC!!!\n";
  init_warning += "\n";

  if(!promtToProceed( init_warning.c_str() ))
  {
    std::cout << "quit." << std::endl;
    return false;
  }

  return true;
}

bool TaskInstallOnFlashDrive::checkFilesize(const std::string& device_name)
{
  // determine the size of the deivce
  uint64_t file_size = getDeviceSize(device_name.c_str());

  if(file_size == 0)
  {
    std::cout << "ERROR: failed to open device!" << std::endl;
    return false;
  }

  // security checks and question to proceed
  uint64_t FILE_SIZE_WARNING_LIMIT = static_cast<uint64_t>(10) * static_cast<uint64_t>(1024) * static_cast<uint64_t>(1024) * static_cast<uint64_t>(1024);

  //std::cout << "file-size = " << file_size << std::endl;
  //std::cout << "file-limit= " << FILE_SIZE_WARNING_LIMIT << std::endl;

  // if the device is bigger than 10GB it is very, very likely that the user
  // tries wanted or accidently to install the image file to a hard disk
  if(file_size >= FILE_SIZE_WARNING_LIMIT)
  {
    std::cout << std::endl;
    std::cout << "SORRY, due to data security reasons it is not possible to copy the image file to a device bigger than 10GB!" << std::endl;
    return false;

    std::string size_warninig = "You are trying to install the SWEB image to a device that is bigger than 10GB.\n";
    size_warninig += "It does NOT look like that the target device is a portable device!";

    if(!promtToProceed( size_warninig.c_str() ))
      return false;
  }

  return true;
}

void TaskInstallOnFlashDrive::execute(void)
{
  if(!printInitialWarning())
    return;

  // determine target name (e.g. dev/sda1)
  std::string device_name = image_util_.getArg(3);

  std::cout << "about to install " << image_util_.getImageInfo()->getFilename() << " to " << device_name << std::endl;

  if(!checkFilesize(device_name))
    return;

  std::string copy_warning = "are you really sure you want to copy the SWEB-image \"";
  copy_warning.append( image_util_.getImageInfo()->getFilename() );
  copy_warning += " to the device " + device_name;

  if(!promtToProceed( copy_warning.c_str() ))
  {
    std::cout << "quit." << std::endl;
    return;
  }

  // copy to device
  if(!copyImageToDevice( device_name.c_str() ))
  {
    std::cout << "ERROR: failed to copy image file to device!" << std::endl;
  }

  std::cout << "SWEB image was successfully copied to device!" << std::endl;
}

char TaskInstallOnFlashDrive::getOptionName(void) const
{
  return 'i';
}

const char* TaskInstallOnFlashDrive::getDescription(void) const
{
  return "makes a one to one copy of the specified virtual-disk image of SWEB on the specified device";
}

uint64_t TaskInstallOnFlashDrive::getDeviceSize(const char* device)
{
  int fd = open(device, O_RDONLY);

  if(fd < 0)
  {
    std::cout << "failed to open " << device << " for reading; check your permissions" << std::endl;
    return 0;
  }

  // loff_t has to be at least 64bit (8byte) in order to deliver reliable values
  assert( sizeof(loff_t) >= 8 );
  if( sizeof(loff_t) < 8 )
  {
    return 0;
  }

  // determine the file-size of the image:
  uint64_t file_size = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);

  close(fd);

  return file_size;
}

bool TaskInstallOnFlashDrive::copyImageToDevice(const char* device)
{
  int dev = open(device, O_WRONLY);

  if(dev < 0)
  {
    std::cout << "failed to open " << device << " for writing; check your permissions" << std::endl;
    return false;
  }

  // getting the ImageInfo object
  ImageInfo* img_info = image_util_.getImageInfo();

  if(img_info == NULL)
  {
    std::cout << "image file is not present!" << std::endl;
    return false;
  }

  // open the image-file for reading
  int img = open(img_info->getFilename(), O_RDONLY);

  if(img < 0)
  {
    std::cout << "WTF? failed to open image!" << std::endl;
    return false;
  }

  const unsigned int SECTOR_SIZE = 512;

  unsigned long num_sectors = img_info->getImageSize() / SECTOR_SIZE;
  if(img_info->getImageSize() % SECTOR_SIZE) num_sectors++;

  char buffer[SECTOR_SIZE];

  // copy the data sector-wise
  for(unsigned long i = 0; i < num_sectors; i++)
  {
    // clear buffer, before reading
    memset(buffer, 0x00, SECTOR_SIZE);

    //std::cout << "copying sector " << i << std::endl;

    // read next sector from img
    if( read(img, buffer, SECTOR_SIZE) <= 0)
    {
      std::cout << "FATAL error, failed to read sector " << i << " from image!" << std::endl;
      break;
    }

    // write sector to device
    if(write(dev, buffer, SECTOR_SIZE) != SECTOR_SIZE)
    {
      std::cout << "FATAL error, failed to write sector " << i << " to device!" << std::endl;
      break;
    }
  }

  close(dev);
  close(img);

  return true;
}
