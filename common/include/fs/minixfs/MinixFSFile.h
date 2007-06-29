// Projectname: SWEB
// Simple operating system for educational purposes

#ifndef MinixFSFile_h___
#define MinixFSFile_h___

#include "fs/File.h"

//-------------------------------------------------------------------------
/**
 * MinixFSFile
 *
 */
class MinixFSFile: public File
{
  public:

   /// constructor of MinixFSFile
    MinixFSFile(Inode* inode, Dentry* dentry, uint32 flag):File(inode,dentry,flag) {};

   /// destructor of MinixFSFile
    virtual ~MinixFSFile(){};

  /// Sets the file position relative to the start of the file, the  end of
  /// the file or the current file position.
    ///
  /// @param offset is the offset to set.
  /// @param origin is the on off SEEK_SET, SEEK_CUR and SEEK_END.
  /// @returns the offset from the start off the file or -1 on failure.
  // l_off_t llSeek(l_off_t offset, uint8 origin)

  /// reads from the file
    ///
  /// @param buffer is the buffer where the data is written to
  /// @param count is the number of bytes to read.
  /// @param offset is the offset to read from counted from the start of the file.
    virtual int32 read(char *buffer, size_t count, l_off_t offset){return 0;};

  /// write to the file
    ///
  /// @param buffer is the buffer where the data is read from
  /// @param count is the number of bytes to write.
  /// @param offset is the offset to write from counted from the start of the file.
    virtual int32 write(const char *buffer, size_t count, l_off_t offset){return 0;};

  /// Open the file
    ///
  /// @param inode is the inode the read the file from.
    virtual int32 open(uint32 flag){return 0;};

  /// Close the file
    ///
  /// @param inode is close, the superblock has the information, that this
  /// inode is not use anymore.
    virtual int32 close(){return 0;};

  /// Flush all off the file's write operations. The File will be written to disk.
    ///
  /// @return is the error code of the flush operation.
    virtual int32 flush(){return 0;};
};

#endif // MinixFSFile_h___
