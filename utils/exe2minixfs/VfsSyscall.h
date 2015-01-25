/**
 * @file VfsSyscall.h
 */

#ifndef VFS_SYSCALL_H___
#define VFS_SYSCALL_H___

#include "MinixFSTypes.h"
#include "PathWalker.h"

class Dirent;
class Dentry;
class VfsMount;
class FileDescriptor;

/**
 * @class VfsSyscall
 */
class VfsSyscall
{
  protected:

    /**
     * checks the duplication from the pathname in the file-system
     * @param pathname the input pathname
     * @return On success, zero is returned. On error, -1 is returned.
     */
    static int32 dupChecking ( const char* pathname, Dentry*& pw_dentry, VfsMount*& pw_vfs_mount );

  public:

    /**
     * make a new directory.
     * i.e. im the path "/file/test/" create a new directory with the name
     * "dir". => the new_dir ist "/file/test/dir"
     * @param pathname the new directory.
     * @param type the permission.
     * @return On success, zero is returned. On error, -1 is returned.
     */
    static int32 mkdir ( const char* pathname, int32 /*type*/ );

    /**
     * The readdir() display the names from all childs and returns a pointer
     * to a Dirent.
     * @param pathname the destination-directory.
     * @return the dirent
     */
    static Dirent* readdir ( const char* pathname );

    /**
     * chdir() changes the current directory to the specified directory.
     * @param dir the destination-directory.
     * @return On success, zero is returned. On error, -1 is returned.
     */
    static int32 chdir ( const char* pathname );

    /**
     * delete a directory, which must be empty.
     * @param pathname the removed directory
     * @return On success, zero is returned. On error, -1 is returned.
     */
    static int32 rmdir ( const char* pathname );

    /**
     * remove a directory (which must be empty) or a file
     * @param pathname the removed directory or file
     * @return On success, zero is returned. On error, -1 is returned.
     */
    static int32 rm ( const char* pathname );

    /**
     * The open() is used to convert a pathname into a file descriptor, if the
     * pathname does not exist, create a new file.
     * @param pathname the file pathname
     * @param flag specified when the file was opened
     * @return On success, file descriptor is returned. On error, -1 is returned.
     */
    static int32 open ( const char* pathname, uint32 flag );

    /**
     * The close() closes a file descriptor.
     * @param fd the file descriptor
     * @return On success, zero is returned. On error, -1 is returned.
     */
    static int32 close ( uint32 fd );

    /**
     * The read() attempts to read up to count bytes from file descriptor fd
     * into the buffer starting at buffter.
     * @param fd the file descriptor
     * @param buffer the buffer that to read the date
     * @param count the size of the byte
     * @return On success, the number of bytes read is returned (zero indicates
     *         end of file), and the file position is advanced by this number.
     *         On error, -1 is returned.
     */
    static int32 read ( uint32 fd, char* buffer, uint32 count );

    /**
     * write  writes  up  to  count  bytes  to the file referenced by the file
     * descriptor fd from the buffer starting at buf.
     * @param fd the file descriptor
     * @param buffer the buffer that to store the date
     * @param count the size of the byte
     * @return On success, the number of bytes written are returned (zero
     *         indicates nothing was written). On error, -1 is returned
     */
    static int32 write ( uint32 fd, const char *buffer, uint32 count );

    /**
     * flushes the file with the given file descriptor to the disc
     * so that changes in the system are written to disc
     * @param fd the file descriptor
     * @return 0 on success, -1 on error
     */
    static int32 flush ( uint32 fd );

    /**
     * returns the size of a file
     * @param fd the file looking for
     * @return the size
     */
    static uint32 getFileSize ( uint32 fd );

    /**
     * get the File descriptor object from the global variable
     * @param the fd int
     * @return the file descriptor object
     */
    static FileDescriptor* getFileDescriptor ( uint32 fd );

  private:
    /**
     * constructor
     */
    VfsSyscall();

    /**
     * destructor
     */
    ~VfsSyscall();
};

#endif // VFS_SYSCALL_H___
