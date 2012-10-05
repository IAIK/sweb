/**
 * Filename: FormatMinixPartition.h
 * Description:
 *
 * Created on: 06.09.2012
 * Author: chris
 */

#ifndef FORMATMINIXPARTITION_H_
#define FORMATMINIXPARTITION_H_

#include "fs/FsDefinitions.h"
#include "fs/device/FsDevice.h"
#include "fs/minix/MinixDefs.h"
#include "fs/minix/MinixDataStructs.h"

#include "types.h"

class FormatMinixPartition
{
public:

  /**
   * formats the given device with MinixFs
   *
   * @param device the device to format with Minix
   * @param minix_version the Version of Minix to format (accepted values are
   * 1 for Version 1 and 2)
   * @param filename_len the length of filenames in Directory entries (14 or 30)
   * @return true in case the Device was successfully formatted; false in case
   * an error occurred
   */
  static bool format(FsDevice* device, sector_addr_t zone_size, int32 minix_version = 1, int32 filename_len = 30);

private:

  /**
   * no instances, no copies !
   */
  FormatMinixPartition();
  virtual ~FormatMinixPartition();

  FormatMinixPartition(const FormatMinixPartition& cpy);

  /**
   * the following static functions are helpers for the format() function
   */
  static bool formatSetupSuperblock(minix_super_block* sb, FsDevice* device,
                                    sector_addr_t zone_size, int32 minix_version,
                                    int32 filename_len);
  static bool formatWriteSuperblock(FsDevice* device, minix_super_block* sb);
  static bool cleanUpDataBlocks(FsDevice* device, sector_addr_t first_sector, sector_addr_t last_sector);
  static bool createRootInode(FsDevice* device, minix_super_block* sb, int32 minix_version, int32 filename_len);
  static bool setRootInodeInBitmaps(FsDevice* device, minix_super_block* sb);
  static bool setRootInodeInTable(FsDevice* device, minix_super_block* sb, int32 minix_version, int32 dir_entry_size);
  static bool setRootInodeInZone(FsDevice* device, minix_super_block* sb, int32 filename_len);
};

#endif /* FORMATMINIXPARTITION_H_ */
