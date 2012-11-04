/*
 * ImageInfo.cpp
 *
 *  Created on: 05.05.2012
 *      Author: chris
 */

#include <iomanip>
#include <sstream>
#include <cstring>

#include "ImageInfo.h"

ImageInfo::ImageInfo(const char* image_file) : image_file_( strdup(image_file) ), file_size_(0)
{
	load(image_file);
}

ImageInfo::~ImageInfo()
{
  if(image_file_ != NULL)
  {
    delete[] image_file_;
    image_file_ = NULL;
  }
  while (partitions_.size() > 0)
  {
    delete partitions_.back();
    partitions_.pop_back();
  }
}

bool ImageInfo::load(const char* image_file)
{
	//
	if(image_file == NULL)
		return false;

	//std::cout << "load(): " << image_file << std::endl;

	std::fstream image(image_file, std::ios::in|std::ios::binary);

	if(image.fail())
	{
		std::cout << "load(): failed to open the image" << std::endl;
		image.close();
		return false;
	}

	// determining the file size:
	image.seekg(0, std::ios_base::end);
	unsigned long image_file_size = image.tellg();
	image.seekg(0, std::ios_base::beg);

	if(image_file_size < BOOT_SECTOR_SIZE)
	{
	  std::cout << "load(): image is too small!" << std::endl;

		// image is definitive use-less!
		image.close();
		return false;
	}

	file_size_ = image_file_size;

	if(!readBootSector(image))
	{
	  std::cout << "load(): failed to read boot-sector" << std::endl;

		image.close();
		return false;
	}

	image.close();
	return true;
}

const char* ImageInfo::getFilename(void) const
{
  return image_file_;
}

unsigned long ImageInfo::getImageSize(void) const
{
  return file_size_;
}

bool ImageInfo::readBootSector(std::fstream& image)
{
	char boot_sector[BOOT_SECTOR_SIZE];
	image.read(boot_sector, BOOT_SECTOR_SIZE);

	if(image.fail())
	{
		std::cout << "readBootSector: read fail" << std::endl;
		return false;
	}

	// checking the boot-sectors signature:
	//if(!(boot_sector[510] == 0x55 && boot_sector[511] == 0xAA))
	if((boot_sector[510] & 0xFF) != 0x55 || (boot_sector[511] & 0xFF) != 0xAA)
	{
		std::cout << "readBootSector: signature corrupt" << std::endl;

		return false;
	}

	partitions_.clear();

	// scanning the partition table
	for(unsigned int i = 0; i < 4; i++)
	{
		PartitionTableEntry* entry = reinterpret_cast<PartitionTableEntry*>(boot_sector + 446+(i*16));

		// skip unused entries
		if(entry->part_type == 0x00
				&& (entry->chs_first_sector[0] == 0 && entry->chs_first_sector[1] == 0 && entry->chs_first_sector[2] == 0)
				&& (entry->chs_last_sector[0] == 0 && entry->chs_last_sector[1] == 0 && entry->chs_last_sector[2] == 0))
		{
			continue;
		}

		partitions_.push_back(new PartitionInfo(entry->boot_able,
											entry->part_type,
											512,
											entry->start_sector_,
											entry->num_sectors_));
	}

	return true;
}

unsigned int ImageInfo::getNumPartitions(void) const
{
	return partitions_.size();
}

const PartitionInfo* ImageInfo::getPartition(unsigned int number) const
{
  if(number > getNumPartitions())
    return NULL;

  return partitions_[number];
}

std::ostream& operator<<(std::ostream& stream, const ImageInfo& image)
{
	//stream << "image: <filename>" << std::endl;
	stream << "size " << image.file_size_ << " bytes - " << ImageInfo::getSizeInBestFittingUnit( image.file_size_ ) << std::endl;
	stream << std::endl;

	stream << std::setw(5) << "part" << std::setw(5) << "boot" << std::setw(15) << "file-system\t" << std::setw(13) << "offset" << std::endl;

	for(unsigned int i = 0; i < image.partitions_.size(); i++)
	{
		stream << std::setw(5) << i
		     << std::setw(5) << (image.partitions_[i]->isBootAble() ? "yes" : "no")
		     << std::setw(15)
		        << PartitionInfo::getPartitionTypeString(image.partitions_[i]->getPartitionIdentfier())
			   << std::setw(13) << image.partitions_[i]->getPartitionSectorOffset()*image.partitions_[i]->getSectorSize()
			      << " bytes" << std::endl;
	}

	stream << std::endl;
	return stream;
}

std::string ImageInfo::getSizeInBestFittingUnit(unsigned long size)
{
  unsigned long cur_size = size;
  unsigned int num_conversions = 0;

  while(cur_size > 1024)
  {
    if(cur_size % 1024 >= 512)
    {
      cur_size /= 1024;
      cur_size += 1;
    }
    else
    {
      cur_size /= 1024;
    }

    num_conversions++;
  }

  std::ostringstream s;
  s << cur_size;

  std::string string = s.str();

  if(num_conversions == 0) string += "Bytes";
  else if(num_conversions == 1) string += "KB";
  else if(num_conversions == 2) string += "MB";
  else if(num_conversions == 3) string += "GB";
  else if(num_conversions == 4) string += "TB";
  else if(num_conversions == 5) string += "EB";
  else
  {
    s.clear();
    s << num_conversions;
    string += "*1024^" + s.str();
  }

  return string;
}
