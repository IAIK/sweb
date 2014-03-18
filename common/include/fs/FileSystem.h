/**
 * Filename: FileSystem.h
 * Description:
 *
 * Created on: 13.05.2012
 * Author: chris
 */

#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include "fs/FsDefinitions.h"
#include "fs/FsBitmap.h"
#include "fs/FileDescriptor.h"
#include "fs/MountFlags.h"
#include "fs/inodes/File.h"
#include "fs/inodes/RegularFile.h"

#include "cache/GeneralCache.h"

#ifdef USE_FILE_SYSTEM_ON_GUEST_OS
#include <sys/types.h>
#endif

// forwards
struct statfs_s;
class FsDevice;
class Inode;
class Directory;

class FsVolumeManager;

/**
 * @class abstract FileSystem
 */
class FileSystem
{
  friend class Inode;
  friend class FsBitmap;
  friend class RegularFile;
public:
  /**
   * creates a new FileSystem instance
   *
   * @param device the underlining device of the FS
   * @param mount_flags the mount-flags
   */
  FileSystem(FsDevice* device, uint32 mount_flags);

  /**
   * unmount / destruct
   */
  virtual ~FileSystem();

  /**
   * getting the partition Identfier Code of the File-System
   * @return the Partition-Identifier
   */
  virtual uint8 getPartitionIdentifier(void) const = 0;

  /**
   * getting the name of the implemented FileSystem
   * @return the fs-name
   */
  virtual const char* getName(void) const = 0;

  /**
   * Fs Error codes
   */
  enum FileSystemErrorCodes
  {
    InvalidArgument       = -1, // invalid argument
    IOReadError           = -2, // I/O read failure
    IOWriteError          = -3, // I/O write failure
    NonBlockAcquire       = -4, // failed to establish mutual exclusion at non-blocking
    FileSystemFull        = -5  // no more free space on the FS
  };

  /**
   * getting the mount flags of the FileSystem
   *
   * @return the mount-flags of the FileSystem
   */
  uint32 getMountFlags(void) const;

  /**
   * creates a new file on the FileSystem
   * @param parent the parent-dir of the new file
   * @param name the name of the new file
   * @param permissions the file's permissions
   * @param uid the owner of the new File
   * @param gid the group-id of the new File
   * @return a pointer to the new craeted File
   */
  virtual File* creat(Directory* parent, const char* name,
                      mode_t permissions, uid_t uid, gid_t gid) = 0;

  /**
   * creates a virtual file on the file system
   * virtual means the file can be accessed by all the Syscalls but it is
   * not stored on the underlining device, it just exists as long as the
   * Fs is mounted. At unmount the virtual I-Node will be destroyed
   * without any backup
   */
  //virtual File* creat_virtual(Directory* parent, const char* name) = 0;

  /**
   * creates a new empty directory on the FileSystem
   * makes the Directory instance available and performs JUST FileSystem
   * related taks; changes to the parent are done by the VfsSyscall!
   *
   * @param parent the parent-dir of the new Directory
   * @param name the name of the new Directory
   * @param cur_time the current timestamp before calling this method
   * @param permissions the new Directorie's permissions
   * @param uid the directorie's owner
   * @param gid the owing group of the new Directory
   * @param[out] new_dir_id the ID of the just created Directory
   * @return 0 in case of success; < 0 in case of error
   */
  virtual int32 mkdir(Directory* parent, const char* name,
                      unix_time_stamp cur_time,
                      mode_t permissions, uid_t uid, gid_t gid,
                      inode_id_t& new_dir_id) = 0;

  /**
   * removes an empty directory
   *
   * @param parent the parent Directory of the Directory to remove
   * @param id the Inode ID of the Directory to remove
   * @param name the filename of sub-directory to remove
   * @return error code; 0 in case of success
   */
  virtual int32 rmdir(Directory* parent, inode_id_t id, const char* name) = 0;

  /**
   * creates a hard link to an existing file
   * @param file the file to be hard-linked
   * @param parent the directory where the hard-link should be stored
   * @param name the new name for the existing file (hard-link's name)
   * @return 0 in case of success, a value < 0 in case of error
   */
  virtual int32 link(File* file, Directory* parent, const char* name) = 0;

  /**
   * removes a hard link from a File
   * if this was the last hard-link to the file, the file will also be removed
   * @param file the file from which the hard link should be taken away
   * @param parent the parent directory of the hard-link
   * @param ref_name the name of the hard-link to be removed
   * @return 0 in case of success, a value < 0 in case of error
   */
  virtual int32 unlink(File* file, Directory* parent, const char* ref_name) = 0;

  /**
   * performs a lookup for an I-Node within a Directory. In contrast to
   * Directory::getInode it will scan the Directory's data area and
   * load *all* child i-nodes if they are not loaded yet.
   * If the Directory's children are already loaded lookup is equivalent
   * to call Directory::getInode from a locked context.
   *
   * lookup() is perfectly thread-safe!
   *
   * @param cur_inode the current Directory to search
   * @param inode_name the name of the searched Inode
   * @return a pointer to found Inode or NULL in case of failure
   */
  virtual Inode* lookup(Directory* cur_inode, const char* inode_name);

  /**
   * syncs the whole fs data
   */
  virtual void sync(void);

  /**
   * synchronizes all changes to the Inode
   *
   * @param inode the I-Node that should be synched with the FileSystem
   * @return 0 is case of success; a value < 0 in case of failure
   */
  virtual int32 fsync(Inode* inode);

  /**
   * returns a new instance of the statfs struct filled with current
   * Statistical data of the file-system
   *
   * @return
   */
  virtual statfs_s* statfs(void) const = 0;

  /**
   * @return a new Directory representing the FileSystem's root-directory
   */
  virtual Directory* getRoot(void);
  //virtual const Directory* getRoot(void) const;

  /**
   * borrows and I-Node from the Cache for following operations (maybe read,
   * write or both). Do not forget to release the I-Node afterwards!
   * I-Node creation or deletion is considered as a Write-Operation
   * All Operations just dealing with the I-Node's data (also including data
   * changes, writes) are considered to be read-operations!
   *
   * @param id the ID of the I-Node to get (for FileSystem not using I-Node IDs set
   * pass value) for details refer to the concrete FS-implementation
   * @param parent the parent Directory holding the requested I-Node
   * @param name the (file)name of the requested I-Node (without the path!)
   * @param acquire_write true for write operations (I-Node deletion), false for
   * read only (data access - data change)
   * @return a pointer to the I-Node or NULL if an error happened
   */
  virtual Inode* acquireInode(inode_id_t id, Directory* parent, const char* name) = 0;

  /**
   * releases a borrowed I-Node
   *
   * @param id the borrowed I-Node that should be returned
   */
  virtual void releaseInode(Inode* inode) = 0;

  /**
   * adds a (re)-write operation, for the data stored in the I-Node object,
   * to the queue
   * @param inode the I-Node data to rewrite to the disk
   */
  virtual bool writeInode(Inode* inode) = 0;

  /**
   * destroys an Inode (but not any of the hard-link references to the inode)
   * frees its id, so that subsequently new created inodes can use it again
   * and marks all occupied data sectors as free
   *
   * @param inode_ro_destroy the Inode to destroy
   * @return true if the Inode was successfully removed / false otherwise
   */
  virtual bool destroyInode(Inode* inode_ro_destroy) = 0;

  /**
   * getting the Block Size of the FileSystem
   *
   * @return the block size in bytes
   */
  virtual sector_len_t getBlockSize(void) const;

  /**
   * getting the size of a *data* block on the FileSystem
   *
   * @return the *data* block size in bytes
   */
  virtual sector_len_t getDataBlockSize(void) const;

  /**
   * calculates the the sector address of the first sector of the given
   * data block
   *
   * @param data_block the the number (address) of the data block
   * @return the first sector address of the given data block address
   */
  virtual sector_addr_t convertDataBlockToSectorAddress(sector_addr_t data_block) = 0;

  /**
   * getting the first sector-address of the first data block
   *
   * @return the sector-address of the first data block
   */
  virtual sector_addr_t getFirstDataBlockAddress(void) const = 0;

  /**
   * allocates a new free block of data on the device for the given
   * file and adds the number of the new sector to the I-Node's sector-list
   *
   * @param inode the I-node which data area should be extended
   * @param zero_out_sector if set to true all bytes of new sector will be
   * set to 0x00; if false all data will be untouched
   * @return true in case of success, false if no more block was available
   */
  virtual sector_addr_t appendSectorToInode(Inode* inode, bool zero_out_sector = false) = 0;

  /**
   * removes the n-th data block from the given I-Node (so that the sector can
   * be used again by other I-Node's). This causes the I-Node's size to shrink.
   * Nevertheless the file-size of the I-Node has to be decreased manually (it
   * is not done by this method!)
   *
   * @param inode the I-Node that should loose it's n-th data-block
   * @param sector_to_remove the number (n-th) sector to remove from the Inode
   * @return the address of the sector that was removed, or 0 in case of error
   */
  virtual sector_addr_t removeSectorFromInode(Inode* inode, uint32 sector_to_remove) = 0;

  /**
   * removes the last data block of the given I-Node (so that the sector can
   * be used again by other I-Node's). This causes the I-Node's size to shrink.
   * Nevertheless the file-size of the I-Node has to be decreased manually (it
   * is not done by this method!)
   *
   * @param inode
   * @return true in case of success / false
   */
  virtual sector_addr_t removeLastSectorOfInode(Inode* inode) = 0;

  /**
   * updates an I-Node's sector list
   * when an i-node is loaded from the device and created as an in-memory
   * object instance there might be some or all of the data sectors be
   * loaded and cached in the object.
   * However if not all of the sectors are loaded (e.g. for performance
   * reasons, they have to be reloaded on request.
   * Inode::getSector() uses this FileSystem specific method to reload
   * one or more missing sectors
   *
   * @param inode the i-node thats list of blocks has to be updated
   */
  virtual void updateInodesSectorList(Inode* inode) = 0;

  /**
   * getting a pointer to the instance of the FileSystem's FsVolumeManager
   *
   * @return a ptr to the FsVolumeManager
   */
  FsVolumeManager* getVolumeManager(void);

protected:

  /**
   * (re)-loads a Directory's children to the memory form the Device
   * unsafe method : ONLY call from a WriteProtected context
   * @param parent the Directory to (re)load
   * @return true in case of success / false otherwise
   */
  virtual bool loadDirChildrenUnsafe(Directory* parent) = 0;

  /**
   * is the given filename a valid one for the implemented file-system?
   * this is a general check including null-bytes and path-separators
   * @param filename
   * @return true if the name is perfectly OK / false otherwise
   */
  virtual bool isFilenameValid(const char* filename, uint32 str_len);

  // the underlining device
  FsDevice* device_;

  // the Volume Manager
  FsVolumeManager* volume_manager_;

  // the Root-Directory (is the only real I-Node NOT in the cache!)
  // evictions of the Root from the Cache would be fatal!
  Directory* root_;

  // the I-Node cache
  Cache::GeneralCache* inode_cache_;

  // the mount-flags
  uint32 mount_flags_;
};

#endif /* FILESYSTEM_H_ */
