/*
 * PartitionInfo.h
 *
 *  Created on: 05.05.2012
 *      Author: chris
 */

#ifndef PARTITIONINFO_H_
#define PARTITIONINFO_H_

#include <string>

/**
 * a Partition info object
 */
class PartitionInfo
{
public:
	/**
	 * constructor
	 * @param boot_able
	 * @param partition_identifier
	 * @param sector_size
	 * @param start_sector start-sector relative to the start of the volume
	 */
	PartitionInfo(bool boot_able, char partition_identifier,
				  unsigned int sector_size, unsigned long start_sector, unsigned long num_sectors);

	virtual ~PartitionInfo();

	bool isBootAble(void) const
	{
		return boot_able_;
	}

	char getPartitionIdentfier() const
	{
		return partition_identifier_;
	}

	unsigned int getSectorSize() const
	{
		return sector_size_;
	}

	/**
	 * returns the offset (starting address) of the Partition
	 * in bytes!
	 * @return offset in bytes!
	 */
	unsigned long getPartitionSectorOffset(void) const
	{
		return start_sector_;
	}

	/**
	 * returns the number of sectors of the Partition
	 * @return
	 */
	unsigned long getNumSectors(void) const
	{
	  return num_sectors_;
	}

	/**
	 * converts the given partition-identifier into the equivalent
	 * file-system name and returns the string
	 * @param part_type
	 * @return fs-name as a string
	 */
	static std::string getPartitionTypeString(unsigned char part_type);

protected:
private:

	// is boot-able?
	bool boot_able_;

	// partition identifier
	char partition_identifier_;

	// the sector size (in bytes of the device)
	unsigned int sector_size_;

	// the offset (in bytes) where the partition starts on the Device
	unsigned long start_sector_;

	// number of sectors per partition
	unsigned long num_sectors_;
};

#endif /* PARTITIONINFO_H_ */
