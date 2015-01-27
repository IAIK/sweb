#ifdef EXE2MINIXFS
#ifndef FILE_H__
#define FILE_H__

#include "types.h"
#include <list>
#include "Inode.h"

// forward declarations
class Superblock;
class Dentry;

/**
 * The basic flags for files
 */
#define O_RDONLY    0x0000
#define O_WRONLY    0x0001
#define O_RDWR      0x0002

/**
 * The basic access modes for files
 */
#define A_READABLE  0x0001
#define A_WRITABLE  0x0002
#define A_EXECABLE  0x0004

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
      {};

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
    uint32 getFlag() {return flag_;}

  public:

    /**
     * The Constructor
     * @param inode the files inode
     * @param dentry the files dentry
     * @param flag the files flag
     */
    File ( Inode* inode, Dentry* dentry, uint32 flag ) : f_superblock_(0), f_inode_(inode), f_dentry_(dentry), flag_(flag)
    {
    }

    /**
     * The Destructor
     */
    virtual  ~File() {}

    /**
     * Getter Method for the dentry.
     * @return is the dentry associated to the File.
     */
    Dentry *getDentry() { return f_dentry_;}

    /**
     * Get Method for the inode.
     * @return is the dentry associated to the inode.
     */
    Inode* getInode() { return f_inode_; }

    /**
     * not implemented here
     * reads from the file
     * @param buffer is the buffer where the data is written to
     * @param count is the number of bytes to read.
     * @param offset is the offset to read from counted from the start of the file.
     */
    virtual int32 read ( char */*buffer*/, size_t /*count*/, l_off_t /*offset*/ )=0;

    /**
     * not implemented here
     * write to the file
     * @param buffer is the buffer where the data is read from
     * @param count is the number of bytes to write.
     * @param offset is the offset to write from counted from the start of the file.
     */
    virtual  int32 write ( const char */*buffer*/, size_t /*count*/, l_off_t /*offset*/ ) = 0;

    /**
     * not implemented here
     * Flush all off the file's write operations. The File will be written to disk.
     * @return is the error code of the flush operation.
     */
    virtual  int32 flush() = 0;

    /**
     * returns the file size
     * @return the size
     */
    uint32 getSize()
    {
      return f_inode_->getSize();
    }
};


#endif
#endif
