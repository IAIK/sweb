// Projectname: SWEB
// Simple operating system for educational purposes

#ifndef MinixFSFile_h___
#define MinixFSFile_h___

#include "fs/File.h"

//-------------------------------------------------------------------------
/**
 * @class MinixFSFile handles files on a minix file system
 * basically read and write
 */
class MinixFSFile: public File
{
  public:

    /**
     * constructor
     * @param inode the inode of the file
     * @param dentry the dentry
     * @param flag the flag i.e. readonly
     */
    MinixFSFile(Inode* inode, Dentry* dentry, uint32 flag);

    /**
     * destructor
     */
    virtual ~MinixFSFile();

    /**
     * reads from the file
     * @param buffer the buffer where the data is written to
     * @param count the number of bytes to read
     * @param offset the offset to read from counted from the start of the file.
     * @return the number of bytes read
     */
    virtual int32 read(char *buffer, size_t count, l_off_t offset);

    /**
     * writes to the file
     * @param buffer the buffer where the data is read from
     * @param count the number of bytes to write
     * @param offset the offset to write from counted from the start of the file
     * @return the number of bytes written
     */
    virtual int32 write(const char *buffer, size_t count, l_off_t offset);

    /**
     * not implemented
     */
    virtual int32 open(uint32 flag);

    /**
     * not implemented
     */
    virtual int32 close();

    /**
     * writes all data to disc
     * @return 0 on success
     */
    virtual int32 flush();
};

#endif // MinixFSFile_h___
