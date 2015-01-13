/**
 * @file Inode.h
 */

#ifndef _INODE_H_INCLUDED_
#define _INODE_H_INCLUDED_

#include "types.h"

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
#include <ulist.h>
#include <umap.h>
#else
#include <vector>
#include <map>
#endif

#include "fs/FsDefinitions.h"
#include "fs/FileSystemLock.h"

// forwards
class Dentry;
class File;
class Superblock;
class Directory;

/**
 * three possible inode state bits:
 */
#define I_UNUSED 0 // the unused inode state
#define I_DIRTY 1 // Dirty inodes are on the per-super-block s_dirty_ list, and
// will be written next time a sync is requested.
#define I_LOCK 2  //state not implemented

/**
 * five possible inode type bits:
 */
#define I_FILE         0
#define I_DIR          1
#define I_LNK          2
#define I_CHARDEVICE   3
#define I_BLOCKDEVICE  4

// forwards
class FileSystem;

/**
 * @class Inode
 * A node in the Virtual File-System tree is called I-node. A I-node is a
 * abstract concept. It's Realizations are Files, Directories, ...
 */
class Inode
{
  public:

    /**
     * possible I-Node types
     */
    enum InodeType
    {
      InodeTypeFile = 1,
      InodeTypeDirectory,
      InodeTypeSpecialFile,   // general special file (for dev use: InodeTypeBlockDevice)
      InodeTypeBlockDevice,   // special-file in the VFS-tree, provides access to a Device
      InodeTypeFSMount,       // a Mount-point to another FS (special Directory)
      InodeTypeVirtual        // a virtual I-Node (->for NFS implement)
    };

    /**
     * full constructor - reference counter will be initialized to 1
     *
     * @param inode_number
     * @param device_sector
     * @param sector_offset
     * @param file_system the FileSystem where the I-Node belongs to
     * @param access_time last time of access
     * @param mod_time last modification time
     * @param c_time creation / last-state change time
     * @param reference_conter
     * @param uid
     * @param gid
     */
    Inode(inode_id_t inode_number,
          sector_addr_t device_sector, sector_len_t sector_offset,
          FileSystem* file_system,
          unix_time_stamp access_time, unix_time_stamp mod_time,
          unix_time_stamp c_time,
          uint32 reference_conter = 1, file_size_t size = 0,
          uint32 uid = 0, uint32 gid = 0);

    /**
     * copy-constructor
     *
     * @param cpy an already existing I-Node object
     */
    Inode(const Inode& cpy);

    /**
     * destructor
     */
    virtual ~Inode();

    /**
     * getting the type of the I-Node
     *
     * @return the I-Node type
     */
    virtual InodeType getType(void) const = 0;

    /**
     * getting access to the Inode Lock-Strategy
     */
    FileSystemLock* getLock(void);

    /**
     * getting a pointer to the File-System where the I-Node is stored on
     * @return a FileSystem pointer
     */
    FileSystem* getFileSystem(void);

    /**
     * getting the I-Node's ID
     *
     * @return the ID of the i-node
     */
    inode_id_t getID(void) const;

    /**
     * sets a new Name for the I-Node
     * @param new_name the new name for the I-Node
     */
    void setName(const char* new_name);

    /**
     * getting the I-Node's name
     *
     * @return the name of the I-Node
     */
    const char* getName(void) const;

    /**
     * increments / decrements the reference counter of the Inode
     */
    void incrReferenceCount(void);
    void decrReferenceCount(void);

    /**
     * getting the reference count to the file
     * @return ref count
     */
    uint32 getReferenceCount(void) const;

    /**
     * getting the file-size
     * @return the file size
     */
    virtual file_size_t getFileSize(void) const;

    /**
     * sets a new File-size
     * @param size new file-size
     */
    void setFileSize(file_size_t size);

    /**
     * @return the last access time of the I-Node
     */
    unix_time_stamp getAccessTime(void) const;

    /**
     * @return the last modification time of the I-Node
     */
    unix_time_stamp getModTime(void) const;

    /**
     * c-time stands either for last-state-change or creation-time
     * as you like ;)
     */
    unix_time_stamp getCTime(void) const;

    /**
     * updates the I-node's time stamps
     *
     * @param updated_time_t the new time to apply
     */
    void updateAccessTime(unix_time_stamp updated_access_t);
    void updateModTime(unix_time_stamp updated_mod_t);
    void updateCTime(unix_time_stamp updated_c_t);

    /**
     * should the Inode's last access time be updated under the current mount
     * flags settings
     *
     * @return true if the access time has to be updated; false if not
     */
    bool doUpdateAccessTime(void) const;

    /**
     * getting the UID / GID of the I-node
     * @return the I-node's owner informations
     */
    uint32 getUID(void) const;
    uint32 getGID(void) const;

    /**
     * setting UID / GID info
     *
     * @param uid
     * @param gid
     */
    void setUID(uint32 uid);
    void setGID(uint32 gid);

    /**
     * setting the Inode permissions in the Unix octal style
     * 0777 for all rights to everyone
     *
     * @param permissions
     */
    void setPermissions(uint32 permissions);

    /**
     * getting the current file-permissions
     *
     * @return Inode permissions
     */
    uint32 getPermissions(void) const;

    /**
     * getting the n-th sector of the I-Node
     * if the sector is missing the method will try to update the i-node's
     * sector list by loading them from the FileSystem
     *
     * @param number the number of the i-node to return
     * @return the sector number or 0 if there is not a sector with the
     * requested number
     */
    sector_addr_t getSector(uint32 number);

    /**
     * getting the n-th sector of the I-Node
     * const version does no reloading of missing sectors
     *
     * @param number the number of the i-node to return
     * @return the sector number or 0 if there is not a sector with the
     * requested number
     */
    sector_addr_t getSector(uint32 number) const;

    /**
     * adds a sector to the end of the linked-sector-list
     * @param sector the number of the following sector
     */
    void addSector(sector_addr_t sector);

    /**
     * removes the last sector from the list
     */
    void removeLastSector(void);

    /**
     * removes the n-th sector from the list
     *
     * @param number the index of the sector to remove
     */
    void removeSector(uint32 number);

    /**
     * getting the number of sectors used by the I-Node
     *
     * @return the number of elements currently stored in the list
     */
    uint32 getNumSectors(void) const;

    /**
     * clears the list of all sectors
     */
    void clearSectorList(void);

    /**
     * sets an indirect block with sectors to the I-Node
     *
     * @param deg_of_indirection
     * @param sector
     */
    void setIndirectBlock(uint32 deg_of_indirection, sector_addr_t sector);

    /**
     * getting the indirect data-block with the given degree of indirection
     *
     * @param deg_of_indirection
     * @return
     */
    sector_addr_t getIndirectBlock(uint32 deg_of_indirection) const;

    /**
     * getting the sector-number where the I-Node data is stored onto
     * an the offset in bytes within the sector
     *
     * NOTE: this is information is file-system implementation dependent and
     * may be used for performance reasons.
     */
    sector_addr_t getDeviceSector(void) const;
    sector_len_t getSectorOffset(void) const;

  private:

    // optional I-Node Number (ID)
    inode_id_t number_;

    // optional I-Node name (alternative identifier instead of ID)
    char* name_;

    // the sector on the device and the offset within the sector
    // where the I-Node data is stored
    sector_addr_t device_sector_;
    sector_len_t sector_offset_;

    // linked-list with block-devices sectors
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
    ustl::list<sector_addr_t> data_sectors_;
#else
    std::vector<sector_addr_t> data_sectors_;
#endif

    // optional list of blocks that refer to data-blocks (indirect address)
    // degree of indirection maps onto the block-address
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
    ustl::map<uint32, sector_addr_t> indirect_blocks_;
#else
    std::map<uint32, sector_addr_t> indirect_blocks_;
#endif

  protected:

    // the lock-mechanism for the Inode's data (for read/write accesses to
    // the I-Node's data NOT for protecting the existence of the Node!)
    FileSystemLock* inode_lock_;

    // the File-System the I-Node is on
    FileSystem* file_system_;

    // access, mod, creation (state-change) times
    unix_time_stamp access_time_;
    unix_time_stamp mod_time_;
    unix_time_stamp c_time_;

    /**
     * The user id of the I-Node
     */
    uint32 uid_;

    /**
     * The group id of the I-Node
     */
    uint32 gid_;

    /**
     * Inode permissions (in style of Unix e.g. 0755)
     */
    uint32 permissions_;

    /**
     * reference counter / number of links to the I-Node
     */
    uint32 reference_count_;

    /**
     * the size / length of the Inode's data area
     */
    file_size_t size_;
};

#endif // _INODE_H_INCLUDED_
