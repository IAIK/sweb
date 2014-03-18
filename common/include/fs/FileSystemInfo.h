/**
 * Filename: FileSystemInfo.h
 * Description:
 *
 * Created on: 12.07.2012
 * Author: chris
 */

#ifndef FILESYSTEMINFO_H_
#define FILESYSTEMINFO_H_

#include "types.h"

// fwd decl
class FsDevice;
class FileSystem;

/**
 * @class FileSystem Info and Factory class giving Informations about a certain Fs
 * and creates and returns a new FS Instance on request
 */
class FileSystemInfo
{
public:
  FileSystemInfo();
  virtual ~FileSystemInfo();

  /**
   * getting the Partition Identifier of the FileSystem
   * created by this Fs-Factory
   * @return partition ident
   */
  virtual uint8 getPartitionIdent(void) const = 0;

  /**
   * getting the name of the FS-created by this Factory
   * @return the name of the FS
   */
  virtual const char* getName(void) const = 0;

  /**
   * getNewFileSystemInstance - creates, confiures and returns a new
   * FileSystem instance on the given device under considering the given
   * mount-flags
   *
   * @param device the FsDevice were the Fs is located on
   * @param mount_flags the mount-flags of the FileSystem
   *
   * @return a new instance of a FileSystem
   */
  virtual FileSystem* getNewFileSystemInstance(FsDevice* device, uint32 mount_flags) = 0;

private:
};

#endif /* FILESYSTEMINFO_H_ */
