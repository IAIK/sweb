/**
 * Filename: Directory.h
 * Description:
 *
 * Created on: 13.05.2012
 * Author: chris
 */

#ifndef DIRECTORY_H_
#define DIRECTORY_H_

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
#include "ustl/ustring.h"
#include "ustl/umap.h"
#include "ustl/ustack.h"
#else
#include <string>
#include <map>
#include <stack>
#endif // USE_FILE_SYSTEM_ON_GUEST_OS

#include "fs/FsDefinitions.h"
#include "fs/inodes/Inode.h"

/**
 * @class special I-Node type - a Directory
 * this class represents a Directory which is loaded into memory
 * NOTE: all Methods which are not named "Safe" should only
 * be called within a locked context (each I-Node provides it's own
 * Mutex)
 */
class Directory : public Inode
{
  public:
    /**
     * class constructor
     *
     * @param inode_number
     * @param device_sector
     * @param sector_offset
     * @param file_system
     * @param access_time
     * @param mod_time
     * @param c_time
     * @param ref_counter
     */
    Directory(inode_id_t inode_number,
              sector_addr_t device_sector, sector_len_t sector_offset,
              FileSystem* file_system,
              unix_time_stamp access_time, unix_time_stamp mod_time,
              unix_time_stamp c_time,
              uint32 ref_counter = 2, file_size_t size = 0);

    /**
     * alternative constructor - without device-related data
     * @param inode_number the Inode ID
     * @param file_system the FileSystem of this Directory
     * @param access_time last access time
     * @param mod_time last modification time
     * @param c_time c-time
     */
    Directory(inode_id_t inode_number, FileSystem* file_system,
              unix_time_stamp access_time,
              unix_time_stamp mod_time,
              unix_time_stamp c_time);

    /**
     * destructor
     */
    virtual ~Directory();

    /**
     * getting the type of the I-Node
     * @return the I-Node type
     */
    virtual InodeType getType(void) const;

    /**
     * getting the number of available children
     * NOTE: just consider this if isDataValid is true AND you are operating
     * within a locked context!!
     * @return number of loaded children
     */
    uint32 getNumChildren(void) const;

    /**
     * getting a I-Node's data by it's hard-link reference
     * within this Directory
     * NOTE: just consider this if isDataValid is true AND you are operating
     * within a locked context!!
     * @param filename the hard-link reference name to the I-Node
     * @return the I-Node or NULL if the filename was not found
     * NOTE: required to be called from a locked context!
     */
    Inode* getInode(const char* filename);

    /**
     * getting an I-Node's data by it's index within the Directory children
     * list
     * @param index the index of the I-Node within the Directorie's list
     * @return a pointer to the I-Node or NULL in case that
     * index >= getNumChildren()
     * NOTE: required to be called from a locked context!
     */
    Inode* getInode(uint32 index);

    /**
     * getting a child Inode, in case the requested Inode refers to a mount-point
     * this method will return the real Inode mount Directory and NOT the
     * Root of the mounted FileSystem
     *
     * @param filename
     * @return the Inode
     */
    Inode* getRealInode(const char* filename);

    /**
     * checks and determines whether the Directory has a child with the given
     * filename
     *
     * @param filename
     * @return true / false
     */
    bool isChild(const char* filename) const;

    /**
     * adds a new child to the cache (only for read-on hardware existing
     * I-Nodes like Files, Sub-Directories or Links)
     * NOTE: required to be called from a locked context!
     * @param filename the hard-link filename reference of the I-Node
     * @param inode_id the ID of the I-Node
     * NOTE: required to be called from a locked context!
     */
    void addChild(const char* filename, inode_id_t inode_id);

    /**
     * removes a child item from the Directory
     * @param filename
     * @return true / false
     */
    bool removeChild(const char* filename);

    /**
     * is the current directory empty?
     * if the directories children are not loaded, they will be loaded
     * before checking if the Directory is empty
     *
     * @return true if the Directory is empty / false if not
     */
    bool isEmpty(void) const;

    /**
     * call this to set the internal flag of the Directory that
     * indicates that all children from the Device-Directory are
     * now in the object's list
     * NOTE: required to be called from a locked context!
     */
    void allChildrenLoaded(void);

    /**
     * clears the Children-Cache
     * NOTE: required to be called from a locked context!
     */
    void clearAllChildren(void);

    /**
     * are the Children loaded?
     * @return true is all children were loaded into the Directorie's list
     * otherwise false
     * NOTE: required to be called from a locked context!
     */
    bool areChildrenLoaded(void) const;

    /**
     * adds a mounted FileSystem to the Directory; if this is the first
     * FileSystem that gets appended the Directory becomes a mount-point
     *
     * @param mounted_fs the mounted fs to push onto the mount-stack
     */
    void pushMountedFs(FileSystem* mounted_fs);

    /**
     * pops the front mounted FileSystem
     */
    bool popMountedFs(void);

    /**
     * getting the currently mounted FS (which is on top of the stack)
     *
     * @return a ptr to the mounted FS or NULL if nothing is mounted
     */
    FileSystem* getMountedFs(void);

    /**
     * is this Directory a mount point - is there at least one FS mounted to it?
     *
     * @return true / false
     */
    bool isMountPoint(void) const;

  protected:

    // are all children of this Directory loaded here in the list?
    bool all_children_loaded_;

    // a mapping from the filename to the Inode data
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
    typedef ustl::map<ustl::string, inode_id_t>::iterator ChildrenIterator;
    ustl::map<ustl::string, inode_id_t> children_;
#else
    typedef std::map<std::string, inode_id_t>::iterator ChildrenIterator;
    std::map<std::string, inode_id_t> children_;
#endif

    // stack of mounted FileSystems
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
    ustl::stack<FileSystem*> mounted_fs_;
#else
    std::stack<FileSystem*> mounted_fs_;
#endif

  private:

    /**
     * obtains a child inode from the FileSystem
     * includes special handling in case the requested child is an active
     * mount-point (the mounted Fs' root will be returned in that case)
     * the parent used fro the request is the this-pointer
     *
     * @param id
     * @param name
     *
     * @return the inode instance (needs to be released after usage)
     */
    Inode* obtainInode(inode_id_t id, const char* name);

};

#endif /* DIRECTORY_H_ */
