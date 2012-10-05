/**
 * Filename: RegularFile.h
 * Description:
 *
 * Created on: 21.07.2012
 * Author: chris
 */

#ifndef REGULARFILE_H_
#define REGULARFILE_H_

#include "File.h"

class FsVolumeManager;

/**
 * @class a RegularFile is the typical file type; it is stored on the
 * device
 */
class RegularFile : public File
{
public:
  /**
   * Regular File constructor
   *
   * @param
   * @param
   */
  RegularFile(uint32 inode_number, uint32 device_sector, uint32 sector_offset,
              FileSystem* file_system, FsVolumeManager* volume_manager,
              unix_time_stamp access_time, unix_time_stamp mod_time, unix_time_stamp c_time,
              uint32 ref_count, uint32 size);

  /**
   * destructor
   */
  virtual ~RegularFile();

  virtual int32 read(FileDescriptor* fd, char* buffer, uint32 len);
  virtual int32 write(FileDescriptor* fd, const char* buffer, uint32 len);

  /**
   * discards all contents of the file without further safety checks
   * and without establishing mutual exclusion!
   * WARNING: if you call this method the contents of the file will
   * be lost forever because this method will add the changed to it's
   * own data to the Write queue of the I-Node cache!
   *
   * @return true / false
   */
  virtual bool truncateUnprotected(void);

private:

  /**
   * updates the last access time of the File in protected mode
   *
   * @param fd the FileDescriptor object
   * @return true / false
   */
  bool updateLastAccessTimeProtected(FileDescriptor* fd);

  /**
   * acquires a write-lock
   */
  int32 lockWrite(FileDescriptor* fd);

  /**
   *
   */
  void unlockWrite(FileDescriptor* fd);

  // the FsVolumeManager of the FileSystem
  FsVolumeManager* volume_manager_;

};

#endif /* REGULARFILE_H_ */
