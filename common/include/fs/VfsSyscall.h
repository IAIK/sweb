/**
 * @file VfsSyscall.h
 */

#ifndef VFS_SYSCALL_H___
#define VFS_SYSCALL_H___

#include "types.h"

#include "fs/FsDefinitions.h"
#include "fs/FileSystemPool.h"
#include "fs/FileSystem.h"

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
#include "kernel/Mutex.h"
#endif

//#include "fs/inodes/Inode.h"
//#include "fs/inodes/Directory.h"

// forwards
class Inode;
class Directory;
class File;

struct statfs_s;
struct DIR;
class Dirent;
class FileDescriptor;

class Thread;

#ifndef NULL
#define NULL    0
#endif

// lseek constants
#define SEEK_SET          0
#define SEEK_CUR          1
#define SEEK_END          2

/**
 * @class VfsSyscall the interface to all FileSystem functions (using POSIX
 * style-functions)
 * use in SWEB VfsSyscall is a Singelton, used on a host OS it's just a normal
 * class (multiple instances are possible), but mounting is NOT possible!
 * @brief VfsSyscall is the interface to the FileSystem
 */
class VfsSyscall
{
  // my friends
  friend class FsWorkingDirectory;

  public:

#ifdef USE_FILE_SYSTEM_ON_GUEST_OS
    /**
     * constructor
     *
     * @param fs_device the FsDevice used by the FileSystem that should be used
     * @param partition_type the partition identifier of the mounted FsDevice
     * @param path_separator the directory-path separator char
     */
    VfsSyscall(FsDevice* fs_device, uint8 partition_type, char path_separator = '/');

    /**
     * destructor
     */
    virtual ~VfsSyscall();
#else
    /**
     * creating the VfsSyscall-interface
     */
    static void createVfsSyscall(void);

    /**
     * getting the pointer to the only existing VfsSyscall (->Singelton)
     */
    static VfsSyscall* instance();
#endif

    /**
     * create a new Directory
     *
     * @param cur_thread the current Thread calling this function
     * @param pathname the new directory.
     * @param mode the permissions.
     * @return On success, zero is returned. On error, -1 is returned.
     */
    virtual int32 mkdir ( Thread* cur_thread, const char* pathname, mode_t mode );

    /**
     * delete a directory, which must be empty.
     * @param cur_thread the current Thread calling this function
     * @param pathname the removed directory
     * @return On success, zero is returned. On error, -1 is returned.
     */
    virtual int32 rmdir ( Thread* cur_thread, const char* pathname );

    /**
     * creates a (hard) link to a file
     *
     * NOTE: TODO link() currently does not update the effected I-Node's
     * time fields!
     *
     * @param cur_thread the current Thread calling this function
     * @param oldpath already existing path to a file
     * @param newpath the new path to the existing file
     * @return
     */
    virtual int32 link(Thread* cur_thread, const char* oldpath,
                       const char* newpath);

    /**
     * removes a hard link from a file and also the file-itself it the
     * file's reference count decreases to 0!
     * @param cur_thread the current Thread calling this function
     * @param pathname
     * @return
     */
    virtual int32 unlink(Thread* cur_thread, const char *pathname);

    /**
     * opens a directory for enumeration of all containing i-nodes
     * @param cur_thread the current Thread calling this function
     * @param name the path to the directory to open
     * @return a pointer to a DIR object or NULL in case of failure
     */
    virtual DIR* opendir(Thread* cur_thread, const char* name);

    /**
     * The readdir() display the names from all childs and returns a pointer
     * to a Dirent.
     * @param cur_thread the current Thread calling this function
     * @param pathname the destination-directory.
     * @return the dirent
     */
    virtual Dirent* readdir ( Thread* cur_thread, DIR* dirp );

    /**
     * resets the position of the directory stream dirp to the beginning of
     * the directory
     * @param cur_thread the current Thread calling this function
     * @param dirp a pointer to a valid DIR-pointer
     * @return none
     */
    virtual void rewinddir(Thread* cur_thread, DIR* dirp);

    /**
     * closing the directory - freeing resources acquired by the dirp-pointer
     * @param cur_thread the current Thread calling this function
     * @param dirp a pointer to a valid DIR-pointer
     * @return true in case of success / false in case of failure
     */
    virtual bool closedir(Thread* cur_thread, DIR* dirp);

    /**
     * chdir() changes the current directory to the specified directory.
     * @param cur_thread the current Thread calling this function
     * @param dir the destination-directory.
     * @return On success, zero is returned. On error, -1 is returned.
     */
    virtual int32 chdir ( Thread* cur_thread, const char* pathname );

    /**
     * getting the current working directory of the given Thread
     * @param cur_thread the current Thread calling this function
     * @return cur_thread's working directory
     */
    virtual const char* getwd(Thread* cur_thread) const;

    /**
     * create a new file or rewrite an existing one
     *
     * @param cur_thread the current Thread calling this function
     * @param pathname the file's pathname
     * @param flag specified when the file was opened
     */
    virtual int32 creat(Thread* cur_thread, const char *path, mode_t mode = 0755);

    /**
     * The open() is used to convert a pathname into a file descriptor, if the
     * pathname does not exist, create a new file.
     * @param cur_thread the current Thread calling this function
     * @param pathname the file pathname
     * @param flag specified when the file was opened
     * @return On success, file descriptor is returned. On error, -1 is returned.
     */
    virtual int32 open(Thread* cur_thread, const char* pathname, int32 flag, mode_t mode = 0755);

    /**
     * The close() closes a file descriptor.
     * @param cur_thread the current Thread calling this function
     * @param fd the file descriptor
     * @return On success, zero is returned. On error, -1 is returned.
     */
    virtual int32 close(Thread* cur_thread, uint32 fd);

    /**
     * The read() attempts to read up to count bytes from file descriptor fd
     * into the buffer starting at buffter.
     * @param cur_thread the current Thread calling this function
     * @param fd the file descriptor
     * @param buffer the buffer that to read the date
     * @param count the size of the byte
     * @return On success, the number of bytes read is returned (zero indicates
     *         end of file), and the file position is advanced by this number.
     *         On error, -1 is returned.
     */
    virtual int32 read ( Thread* cur_thread, fd_size_t fd, char* buffer, size_t count );

    /**
     * Sets the file position relative to the start of the file, the end of the
     * file or the current file position.
     * @param cur_thread the current Thread calling this function
     * @param fd the file descriptor
     * @param offset is the offset to set.
     * @param origin is the on off SEEK_SET, SEEK_CUR and SEEK_END.
     * @returns the offset from the start off the file or -1 on failure.
     */
    l_off_t lseek ( Thread* cur_thread, fd_size_t fd, l_off_t offset, uint8 origin );

    /**
     * write  writes  up  to  count  bytes  to the file referenced by the file
     * descriptor fd from the buffer starting at buf.
     * @param cur_thread the current Thread calling this function
     * @param fd the file descriptor
     * @param buffer the buffer that to store the date
     * @param count the size of the byte
     * @return On success, the number of bytes written are returned (zero
     *         indicates nothing was written). On error, -1 is returned
     */
    virtual int32 write ( Thread* cur_thread, fd_size_t fd, const char *buffer, size_t count );

    /**
     * commit buffer cache to disk (http://linux.die.net/man/2/sync)
     * sync() causes all buffered modifications to file metadata and data
     * to be written to the underlying file systems.
     */
    virtual void sync(Thread* cur_thread);

    /**
     * fsync() - synchronize a file's in-core state with storage device
     * @param fd the file-descriptor the File to be flushed
     * @return in case of success 0, otherwise -1
     */
    int32 fsync(Thread* cur_thread, int32 fd);

    /**
     * get file system statistics
     *
     * NOTE: the caller is responsible for freeing the data returned by this
     * method
     *
     * @param path
     * @return the statistics of the FileSystem on witch the I-Node specified by
     * path is stored on
     * NULL in case of failure
     */
    statfs_s* statfs(Thread* cur_thread, const char *path);

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS

    /**
     * mounts a file system
     * @param cur_thread the current Thread calling this function
     * @param device_name the device name i.e. ida
     * @param dir_name the directory name where to mount the filesystem
     * @param file_system_name the file system name i.e. minixfs
     * @param flag the flag indicates if mounted readonly etc.
     * @return 0 on success
     */
    virtual int32 mount ( Thread* cur_thread, const char *device_name, const char *dir_name, const char *file_system_name, int32 flag );

    /** unmounts a filesystem
     * @param cur_thread the current Thread calling this function
     * @param dir_name the directory where the filesystem to unmount is mounted
     * @param flag not used
     * @return 0 on success
     */
    virtual int32 umount ( Thread* cur_thread, const char *dir_name, int32 flag );


    /**
     * unmountes the root-file system (recusivley so all still mounted
     * sub-filesystems will be unmounted before)
     *
     */
    virtual void unmountRoot(void);
#endif

    /**
     * getting the current system time stamp (in unix-format - passed seconds since
     * 1.1.1970)
     * @return the current unix timestamp
     */
    static unix_time_stamp getCurrentTimeStamp(void);

    /**
     * returns the last definied path separator
     * WARNING: this might not match the used path-separtor of
     * your used VfsSyscall instance
     * This is just a dirty hack!
     */
    static char getLastDefinedPathSeparator(void);

    /**
     * returns the size of a file
     * @param fd the file looking for
     * @return the size
     */
    //virtual uint32 getFileSize ( uint32 fd );

    /**
     * getting the Root-Directory of the Virtual File System
     * @return the top root Directory of the VFS
     */
    virtual Directory* getVfsRoot(void);

    /**
     * getting the currently-used path-separator
     * @return the path separator-char
     */
    char getPathSeparator(void) const;

  protected:

    /**
     * resolves a string-given path and returns the Inode
     *
     * @param cur_thread the current Thread calling this function
     * @param path the path to resolve (coded as a string)
     * @return the Inode of the reolved path or NULL in case of failure
     */
    virtual Inode* resolvePath(Thread* cur_thread, const char* path);

    /**
     * resolves a string-given path into a Directory
     * in contrast to the more general method resolvePath, resolveDirectory
     * will fail if any part of the path-string does not refer to a Directory
     *
     * @param cur_thread the current Thread calling this function
     * @param path the path to resolve (coded as a string)
     * @return the resolved Directory or NULL in case of error
     */
    virtual Directory* resolveDirectory(Thread* cur_thread, const char* path);

    /**
     * resolves a string-given path into a File
     * in contrast to the more general method resolvePath, resolveFile will
     * fail if the last part of the path-string does not refer to a File
     *
     * @param cur_thread the current Thread calling this function
     * @param path the path to resolve (coded as a string)
     * @return the resolved Directory or NULL in case of error
     */
    virtual File* resolveFile(Thread* cur_thread, const char* path);

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
    /**
     * resolves a string-given path into the real Directory
     * real means that if the path refers to a mount-point not the top-most
     * mount-point of the mounted FileSystem but the real Directory will be
     * returned
     *
     * @param cur_thread the current Thread calling this function
     * @param path the path to resolve (coded as a string)
     * @return the resolved Directory or NULL in case of error (e.g. path does
     * not refer to a Directory)
     */
    virtual Directory* resolveRealDirectory(Thread* cur_thread, const char* path);
#endif

    // possible file-system operations
    enum FileSystemOperations { READ, WRITE, EXECUTE, MOUNT };

    /**
     * HOOK; not implemented
     *
     * checks if the current-Thread is allowed to perform the given operation
     * (READ, WRITE, EXECUTE) on the given I-Node
     *
     * @param cur_thread the current Thread calling this function
     * @param inode the Inode to operate on
     * @param operation the Operations to carry out
     * @return true if the Thread is allowed to carry out the given operation / false
     * if not
     */
    virtual bool isOperationPermitted(Thread* cur_thread, Inode* inode, int32 operation);

    /**
     * HOOK; not implemented
     * checking User's disk-quota
     * @param cur_thread the current Thread calling this function
     * @return true / false if the Disk quota reached its limit (not write's possible)
     */
    virtual bool diskQuotaNotExceeded(Thread* cur_thread);

    /**
     * splitting a given path into the directory and the last-part (file)
     * NOTE: function allocates memory for the path and part, but DOES NOT
     * free the memory!
     * @param[in] full_path the given path to split
     * @param[out] path the directory-path
     * @param[out] part the last-part of the given full_path
     * @return true if the splitting was successful and the output arguments where
     * correctly set / false if the operation failed, in this case do NOT consider
     * the output-arguments
     */
    bool splitPath(const char* full_path, char** path, char** part) const;

    /**
     * extracts the directory-path from a given full path (cutting away the
     * last-part of the given full_path)
     * NOTE: function allocates memory for the path and part, but DOES NOT
     * free the memory!
     * @param full_path the given path to split
     * @return the extracted path or NULL in case of failure
     */
    char* extractsPath(const char* full_path) const;

    /**
     * extracts the last-part from a given full path (cutting away the
     * directory-path of the given full_path)
     * NOTE: function allocates memory for the path and part, but DOES NOT
     * free the memory!
     * @param full_path the given path to split
     * @return the extracted last-part or NULL in case of failure
     */
    char* extractsPart(const char* full_path) const;

    // the separator char for the file-system
    const char PATH_SEPARATOR;

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
    // the mounted file-systems
    ustl::vector<FileSystem*> mounted_fs_;

    // the Lock for the mounted file-systems data-structure
    Mutex mount_lock_;
#endif

    // the root file-system (= the file-system that is mounted to the root)
    // NOTE: the root-filesystem needs no lock because a root-unmount is
    // only performed at shut-down
    FileSystem* root_;

  private:

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
    /**
     * constructor
     * @param path_separator the directory-path separator char
     */
    VfsSyscall(char path_separator = '/');

    /**
     * destructor
     */
    virtual ~VfsSyscall();
#endif

    /**
     * loads and creates the root-filesystem
     */
    int32 initRootFs(void);

    /**
     * returns the position of the last Separator-char in a given path
     * @param full_path the path to search for the Separator-char
     * @return the position the last Separator-char
     */
    uint32 getLastSeparatorPos(const char* full_path) const;

    enum FailCondition { DO_NOT_FAIL, FAIL_IF_CHILD_EXISTS, FAIL_IF_CHILD_DOES_NOT_EXIST };
    /**
     * resolve the parent Directory of the full given path, if the resolution of
     * the path was successful the parent-directory will be locked for WRITE
     * operations, it will be checked whether the child Inode exists in the
     * parent and if the caller has WRITE-access to the parent
     *
     * @param cur_thread the calling current Thread
     * @param pathname
     * @param[out] parent
     * @param[out] last_part
     * @param[out] file_exists
     *
     * @return true if the steps mentioned above were all executed successfully
     * or false in case of error; in case of failure the output parameters will
     * NOT be filled with data!
     */
    bool resolveAndWriteLockParentDirectory(Thread* cur_thread, const char* pathname, FailCondition fail_if,
                                            Directory*& parent, char*& last_part, bool& file_extists);

    /**
     * creates a new File
     *
     * @param cur_thread
     * @param pathname the path of the new file
     * @param mode the file permissions
     */
    File* createFile ( Thread* cur_thread, const char* pathname, mode_t mode );

    /**
     * creates a new FileDescriptor object for a given File considering the given
     * flags (usually only used for the open() method)
     * if the creation of the FD fails the given File will be released by the
     * function
     *
     * @param cur_thread
     * @param file the File to create a FileDescriptor
     * @param flags the open flags
     * @return the created FD ready to be used or NULL in case of failure
     */
    FileDescriptor* createFDForFile(Thread* cur_thread, File* file, uint32 flags);

    /**
     * rewrites the Inode of a parent-directory that was affected by
     * some changes (e.g. creation / removing of sub-directory / file)
     * this method takes care of the MS_DIRSYNC mount-flag
     * NOTE: this
     *
     * @param fs the FileSystem of the parent Directory
     * @param parent_dir the Directory to re-write
     */
    void rewriteParentDirectory(FileSystem* fs, Directory* parent) const;

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
    // the only existing VfsSyscall (Interface) instance
    static VfsSyscall* instance_;
#endif

    // the last definied, used path separator
    static char LAST_DEFINED_PATH_SEPARATOR;

    // the FileSystem pool containing all available file-systems
    FileSystemPool fs_pool_;
};

#endif // VFS_SYSCALL_H___
