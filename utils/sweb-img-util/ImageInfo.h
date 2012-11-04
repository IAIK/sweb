/*
 * ImageInfo.h
 *
 *  Created on: 05.05.2012
 *      Author: Christopher Walles
 */

#ifndef IMAGEINFO_H_
#define IMAGEINFO_H_

#include <stdint.h>
#include <vector>
#include <fstream>


class PartitionInfo;

/**
 * @class collects and returns informations about a given raw image file
 * @brief raw hdd-image file info class
 */
class ImageInfo
{
public:
	/**
	 * constructor
	 * @param image_file null-terminated string with the path to an hdd-image
	 */
	ImageInfo(const char* image_file);

	/**
	 * destructor
	 */
	virtual ~ImageInfo();

	/**
	 * loads an images file's data
	 * @param image_file
	 * @return true / false
	 */
	bool load(const char* image_file);

	/**
	 * getting the filename of the image
	 * @return filename of the image
	 */
	const char* getFilename(void) const;

	/**
	 * getting Partition infos
	 */
	unsigned int getNumPartitions(void) const;

	/**
	 * getting the PartitionInfo object of the specified partition
	 *
	 * @return reference to the PartitionInfo object
	 */
	const PartitionInfo* getPartition(unsigned int number) const;

	/**
	 * getting the file size of the image in bytes
	 *
	 * @return the filesize of the image
	 */
	unsigned long getImageSize(void) const;

	/**
	 * ostream operator
	 * @param stream
	 * @return
	 */
	friend std::ostream& operator<<(std::ostream& stream, const ImageInfo& image);

	/**
	 *
	 */
	static std::string getSizeInBestFittingUnit(unsigned long size);

	struct PartitionTableEntry
	{
		int8_t boot_able;
		int8_t chs_first_sector[3];
		int8_t part_type;
		int8_t chs_last_sector[3];
		int32_t start_sector_;
		int32_t num_sectors_;
	};

protected:
	// ...

private:

	// the default size of the boot-sector
	static const unsigned int BOOT_SECTOR_SIZE = 512;

	bool readBootSector(std::fstream& image);

	// image filename
	char* image_file_;

	// image size
	unsigned long file_size_;

	// the detected partitions:
	std::vector<PartitionInfo*> partitions_;
};

#endif /* IMAGEINFO_H_ */
