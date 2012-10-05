/**
 * Filename: FsBitmap.h
 * Description:
 *
 * Created on: 16.05.2012
 * Author: Christopher Walles
 */

#ifndef FSBITMAP_H_
#define FSBITMAP_H_

#include "FsDefinitions.h"

// forwards
class FileSystem;
class FsVolumeManager;

/**
 * @class File-System Bitmap
 * The FsBitmap is perfectly ThreadSafe!
 */
class FsBitmap
{
public:
  /**
   * FsBitmap constructor
   *
   * @param file_system the FileSystem where the Bitmap is stored on
   * @param fs_volume_manager the FsVolumeManager of the FileSystem
   * @param start_sector number of the first sector of the Device-Bitmap
   * @param end_sector number of the last sector of the Device-Bitmap
   * @param num_bits the number of bits stored by the Bitmap
   */
  FsBitmap(FileSystem* file_system, FsVolumeManager* fs_volume_manager,
           sector_addr_t start_sector, sector_addr_t end_sector, bitmap_t num_bits);

  /**
   * class destructor
   */
  virtual ~FsBitmap();

  /**
   * sets a Bit in the Fs-Bitmap
   * @param index the Index value of the Bit
   * @param value the Bit value to apply
   * @return true in case of success, false in case of error
   */
  bool setBit(bitmap_t index, bool value);

  /**
   * getting the state of a Bit in the Fs-Bitmap
   * @param index
   * @return the state of the Bit
   */
  bool getBit(bitmap_t index);

  /**
   * searches, occupies and returns the number of the next
   * free Bit in the Bitmap
   *
   * @return the number of the next free bit in the Bitmap
   */
  bitmap_t occupyNextFreeBit(void);

  /**
   * statistical method
   * determines and returns the number of free bits in the FsBitmap
   *
   * @return current number of free bits
   */
  bitmap_t getNumFreeBits(void) const;

private:

  // the associated FileSystem where the Bitmap is stored
  FileSystem* file_system_;

  // the VolumeManager
  FsVolumeManager* volume_manager_;

  // the starting sector on the device
  const sector_addr_t start_sector_;

  // number of stored bits
  const bitmap_t num_bits_;

  // number of blocks used by the Bitmap
  const sector_addr_t num_blocks_;
};

#endif /* FSBITMAP_H_ */
