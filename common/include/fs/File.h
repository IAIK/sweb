#ifndef FILE_H__
#define FILE_H__

#include "types.h"

// forward declarations
class Superblock;
class Inode;
class Dentry;

/**
 * The basic flags for files
 */
#define O_RDONLY    0x0000
#define O_WRONLY    0x0001
#define O_RDWR      0x0002
#define O_CREAT     0x0004

/**
 * The basic access modes for files
 */
#define A_READABLE  0x0001
#define A_WRITABLE  0x0002
#define A_EXECABLE  0x0004


/**
 * Seek positioners
 */
#ifndef SEEK_SET
#define SEEK_SET 0
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif
#ifndef SEEK_END
#define SEEK_END 2
#endif

/**
 * @class File
 * A file object is used where-ever there is a need to read from or write to
 * something. This includes accessing objects within file-system. Files are
 * accessible to processes through their file descriptors.
 */
class File
{
  public:

    typedef uint32 mode_t;

    /**
     * @class Owner
     */
    class Owner
    {
    };

    /**
     * The user id of the file
     */
    uint32 uid;

    /**
     * The group id of the file
     */
    uint32 gid;

    /**
     * interna version number.
     */
    uint32 version;

  protected:

    /**
     * The superblock  pointing to this file
     */
    Superblock* f_superblock_;

    /**
     * The indoe associated to the file.
     */
    Inode* f_inode_;

    /**
     * The dentry pointing to this file/
     */
    Dentry* f_dentry_;

    /**
     * The flags specified when the file was opened
     */
    uint32 flag_;

    /**
     * The process access mode of the file;
     * default value: READABLE ^ WRITABLE ^ EXECABLE
     */
    mode_t mode_;

    /**
     * Current offset in the file
     */
    l_off_t offset_;

    /**
     * indicates the owner of the file;
     */
    Owner owner;

  public:
    /**
     * returns the files flag
     * @return the flag
     */
    uint32 getFlag()
    {
      return flag_;
    }

  public:

    /**
     * The Constructor
     * @param inode the files inode
     * @param dentry the files dentry
     * @param flag the files flag
     */
    File(Inode* inode, Dentry* dentry, uint32 flag);

    virtual ~File()
    {
    }

    Dentry *getDentry()
    {
      return f_dentry_;
    }
    Inode* getInode()
    {
      return f_inode_;
    }

    /**
     * Sets the file position relative to the start of the file, the end of the
     * file or the current file position.
     * @param offset is the offset to set.
     * @param origin is the on off SEEK_SET, SEEK_CUR and SEEK_END.
     * @returns the offset from the start off the file or -1 on failure.
     */
    l_off_t lseek(l_off_t offset, uint8 origin);

    /**
     * not implemented here
     * reads from the file
     * @param buffer is the buffer where the data is written to
     * @param count is the number of bytes to read.
     * @param offset is the offset to read from counted from the current file position.
     */
    virtual int32 read(char */*buffer*/, size_t /*count*/, l_off_t /*offset*/)
    {
      return 0;
    }

    /**
     * not implemented here
     * write to the file
     * @param buffer is the buffer where the data is read from
     * @param count is the number of bytes to write.
     * @param offset is the offset to write from counted from the current file position
     */
    virtual int32 write(const char */*buffer*/, size_t /*count*/, l_off_t /*offset*/)
    {
      return 0;
    }

    /**
     * Opens the file
     * @param inode is the inode the read the file from.
     */
    virtual int32 open(uint32)
    {
      return 0;
    }

    /**
     * not implemented here
     * Close the file
     * @param inode is close, the superblock has the information, that this
     * inode is not use anymore.
     */
    virtual int32 close()
    {
      return 0;
    }

    /**
     * not implemented here
     * Flush all off the file's write operations. The File will be written to disk.
     * @return is the error code of the flush operation.
     */
    virtual int32 flush()
    {
      return 0;
    }

    /**
     * returns the file size
     * @return the size
     */
    virtual uint32 getSize();
};

#endif

