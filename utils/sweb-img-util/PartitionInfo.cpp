/*
 * PartitionInfo.cpp
 *
 *  Created on: 05.05.2012
 *      Author: Christopher Walles
 */

#include <sstream>
#include "PartitionInfo.h"

PartitionInfo::PartitionInfo(bool boot_able, char partition_identifier,
		  unsigned int sector_size, unsigned long start_sector, unsigned long num_sectors)
    : boot_able_(boot_able),
		  partition_identifier_(partition_identifier), sector_size_(sector_size),
		  start_sector_(start_sector), num_sectors_(num_sectors)
{
}

PartitionInfo::~PartitionInfo()
{
}

const char* PartitionInfo::getPartitionTypeString(unsigned char part_type)
{
	switch(part_type)
	{
	case 0x00:
		return "empty";
	case 0x01:
		return "FAT12";
	case 0x04:
		return "FAT16 <= 32MiB";
	case 0x05:
		return "extended Partition";
	case 0x06:
		return "FAT16 > 32MiB";
	case 0x07:
		return "NTFS";
	case 0x0B:
		return "FAT32";

	case 0x41:
		return "Minix";

	case 0x81:
		return "Minix";
	case 0x82:
		return "Linux Swap";
	case 0x83:
		return "Linux native";

	default:
		// by default return the partition identifier as a string
		std::stringstream str_stream;
		str_stream << std::hex << ((int)part_type & 0xFF) << std::dec;
		return str_stream.str().c_str();
	}
	return "unknown fs";
}
