/**
 * @file File.h
 */

#ifndef FILE_H__
#define FILE_H__

#include "types.h"

#include "Inode.h"

class FileDescriptor;

/**
 * @class File
 * This class offers all data required to read / write data from / to a file
 * stored on a Device
 * Sepcial kind of file; using 32bit addresses
 */
class File : public Inode
{
  public:

    /**
     * The Constructor
     * @param inode the files inode
     * @param ref_count
     * @param size
     */
    File(uint32 inode_number, uint32 device_sector, uint32 sector_offset,
        FileSystem* file_system, unix_time_stamp access_time,
        unix_time_stamp mod_time, unix_time_stamp c_time,
        uint32 ref_count, uint32 size);

    /**
     * The Destructor
     */
    virtual ~File();

    /**
     * getting the type of the I-Node
     * @return the I-Node type
     */
    virtual InodeType getType(void) const;

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
    virtual int32 read(FileDescriptor* fd, char* buffer, uint32 len) = 0;

    /**
     * writes data to the file
     *
     * @param fd
     * @param buffer
     * @param len
     * @return
     */
    virtual int32 write(FileDescriptor* fd, const char* buffer, uint32 len) = 0;

    /**
     * discards all contents of the file without further safety checks
     * and without establishing mutual exclusion!
     * WARNING: if you call this method the contents of the file will
     * be lost forever because this method will add the changed to it's
     * own data to the Write queue of the I-Node cache!
     *
     * @return true / false
     */
    virtual bool truncateUnprotected(void) = 0;

    /**
     * does the same as truncateUnprotected() but with one difference
     * it will acquire the File's Lock for Writing operation in blocking
     * mode.
     *
     * @return true / false
     */
    virtual bool truncateProtected(void);
};

#endif

