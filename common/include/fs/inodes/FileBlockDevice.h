/**
 * Filename: FileBlockDevice.h
 * Description:
 *
 * Created on: 06.06.2012
 * Author: chris
 */

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS

#ifndef FILEBLOCKDEVICE_H_
#define FILEBLOCKDEVICE_H_

#include "File.h"
#include "arch_bd_virtual_device.h"

/**
 * @class Special-File BlockDevice, this class makes a BlockDevice accessible
 * via the VFS-tree
 * TODO implement!
 */
class FileBlockDevice : public File
{
public:
  FileBlockDevice(BDVirtualDevice* block_dev);
  virtual ~FileBlockDevice();

  /**
   * MUST BE IMPLEMENTED
   * getting the type of the I-Node
   * @return the I-Node type
   */
  virtual InodeType getType(void) const;

  /**
   * getting the Block-Device
   * @return a pointer to the BDVirtualDevice instance
   */
  BDVirtualDevice* getDevice(void);
  const BDVirtualDevice* getDevice(void) const;

  /**
   * read data from the file
   *
   * @param fd the associated FileDescriptor object
   * @param buffer the buffer to store the results of the reading to
   * @param len the length of the buffer and the maximum number of bytes
   * to read
   * @return the number of bytes successfully read or a negative value
   * in case of error
   *
   */
  virtual int32 read(FileDescriptor* fd, char* buffer, uint32 len);

  /**
   * writes data to the file
   *
   * @param fd
   * @param buffer
   * @param len
   * @return
   */
  virtual int32 write(FileDescriptor* fd, const char* buffer, uint32 len);

private:

  // the Block-device associated with
  BDVirtualDevice* block_dev_;
};

#endif /* FILEBLOCKDEVICE_H_ */

#endif // USE_FILE_SYSTEM_ON_GUEST_OS
