/**
 * Filename: FileSystemPool.h
 * Description:
 *
 * Created on: 12.07.2012
 * Author: chris
 */

#ifndef FILESYSTEMPOOL_H_
#define FILESYSTEMPOOL_H_

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
#include "uvector.h"
#else
#include <vector>
#endif

class FsDevice;
class FileSystem;
class FileSystemInfo;

/**
 * @class the FileSystemPool manages all file-system that are fully implemented
 * and supported by SWEB
 * The Client (usually the VfsSyscall-Singleton) that wants to mount a new FileSystem
 * can request here a new FS-instance
 */
class FileSystemPool
{
public:
  /**
   * FileSystemPool constructor is called at Operating System initialization
   * before mounting the root-filesystem
   * NOTE: if you add your own FileSystem implementation - in order to be able
   * to mount that system - add an instance of its FileSystemInfo object here
   */
  FileSystemPool();

  /**
   * destructor
   */
  virtual ~FileSystemPool();

  /**
   * creates, configures and returns a new FileSystem instance which is located
   * on the given FsDevice
   *
   * @param device
   * @param part_ident
   * @param mnt_flags
   *
   * @return the new FileSystem instance or NULL in case of failure
   */
  FileSystem* getNewFsInstance(FsDevice* device, uint8 part_ident, uint32 mnt_flags);
  FileSystem* getNewFsInstance(FsDevice* device, const char* fs_name, uint32 mnt_flags);

  FileSystemInfo* getFsInfo(uint8 fs_name);
  FileSystemInfo* getFsInfo(const char* fs_name);

private:

  // due to the fact that the List is a read only one and at
  // Initialization time just one "Thread" is running, no locking
  // is required
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  ustl::vector<FileSystemInfo*> supported_file_systems_;
#else
  std::vector<FileSystemInfo*> supported_file_systems_;
#endif
};

#endif /* FILESYSTEMPOOL_H_ */
