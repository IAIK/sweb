// Projectname: SWEB
// Simple operating system for educational purposes

#ifndef RamFsFile_h___
#define RamFsFile_h___

#include "fs/File.h"

//-------------------------------------------------------------------------
/**
 * RamFsFile
 *
 */
class RamFsFile: public File
{
 public:

   /// constructor of RamFsFile
  RamFsFile(Inode* inode, Dentry* dentry);

   /// destructor of RamFsFile
  virtual ~RamFsFile();

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
  virtual int32 read(int32 *buffer, size_t count, l_off_t offset);

  /// write to the file
  ///
  /// @param buffer is the buffer where the data is read from
  /// @param count is the number of bytes to write.
  /// @param offset is the offset to write from counted from the start of the file.
  virtual int32 write(int32 *buffer, size_t count, l_off_t offset);

  /// Open the file
  ///
  /// @param inode is the inode the read the file from.
  virtual int32 open(uint32 flag);

  /// Close the file
  ///
  /// @param inode is close, the superblock has the information, that this
  /// inode is not use anymore.
  virtual int32 close();

  /// Flush all off the file's write operations. The File will be written to disk.
  /// 
  /// @return is the error code of the flush operation.
  virtual int32 flush();
};

#endif // RamFsFile_h___
