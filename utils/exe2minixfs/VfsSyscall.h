/**
 * @file VfsSyscall.h
 */

#ifndef VFS_SYSCALL_H___
#define VFS_SYSCALL_H___

#include "types.h"
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
