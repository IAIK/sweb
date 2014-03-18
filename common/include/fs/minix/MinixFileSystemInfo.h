/**
 * Filename: MinixFileSystemInfo.h
 * Description:
 *
 * Created on: 12.07.2012
 * Author: chris
 */

#ifndef MINIXFILESYSTEMINFO_H_
#define MINIXFILESYSTEMINFO_H_

#include "FileSystemInfo.h"

class MinixFileSystemInfo : public FileSystemInfo
{
public:
  MinixFileSystemInfo();
  virtual ~MinixFileSystemInfo();

  /**
   * getting the Partition Identifier of the FileSystem
   * created by this Fs-Factory
   * @return partition ident
   */
  virtual uint8 getPartitionIdent(void) const;

  /**
   * getting the name of the FS-created by this Factory
   * @return the name of the FS
   */
  virtual const char* getName(void) const;

  /**
   * @return a new instance of a FileSystem
   */
  virtual FileSystem* getNewFileSystemInstance(FsDevice* device, uint32 mount_flags);

};

#endif /* MINIXFILESYSTEMINFO_H_ */
