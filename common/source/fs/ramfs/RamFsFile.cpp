// Projectname: SWEB
// Simple operating system for educational purposes

#include "fs/ramfs/RamFsInode.h"
#include "fs/ramfs/RamFsFile.h"
#include "fs/ramfs/RamFsSuperblock.h"
#include "fs/Dentry.h"

#define ERROR_FRO "ERROR: The flag muss be READONLY for several opened files"
#define ERROR_FF  "ERROR: The flag does not allow this operation"
#define ERROR_FNO "ERROR: The file is not open."

//--------------------------------------------------------------------------
RamFsFile::RamFsFile(Inode* inode, Dentry* dentry, uint32 flag) : 
  File(inode, dentry, flag)
{
  f_superblock_ = inode->getSuperblock();
  mode_ = (A_READABLE ^ A_WRITABLE) ^ A_EXECABLE;
  offset_ = 0;
}

//--------------------------------------------------------------------------
RamFsFile::~RamFsFile()
{
}

//--------------------------------------------------------------------------
int32 RamFsFile::read(char *buffer, size_t count, l_off_t offset)
{
  if((flag_ == O_RDONLY) || (flag_ == O_RDWR))
    return(f_inode_->readData(offset, count, buffer));
  else
  {
    // ERROR_FF
    return -1;
  }   
}

//--------------------------------------------------------------------------
int32 RamFsFile::write(const char *buffer, size_t count, l_off_t offset)
{
  if((flag_ == O_WRONLY) || (flag_ == O_RDWR))
    return(f_inode_->writeData(offset, count, buffer));
  else
  {
    // ERROR_FF
    return -1;
  }
}

//--------------------------------------------------------------------------
int32 RamFsFile::open(uint32 flag)
{

  if(f_inode_->openedFilesEmpty() == true)
  {
  }
  else if((f_inode_->insertOpenedFiles(this) == 0) && (flag == O_RDONLY))
  {
  }
  else
  {
    // ERROR_FRO
    return -1;
  }
  
  f_inode_->insertOpenedFiles(this);
  flag_ = flag;
  //f_superblock_->insertOpenedFiles(this);

  return 0;
}

//--------------------------------------------------------------------------
int32 RamFsFile::close()
{
  assert((f_inode_->removeOpenedFiles(this) != 0));// &&
//         (f_superblock_->removeOpenedFiles(this) != 0));

}

//--------------------------------------------------------------------------
int32 RamFsFile::flush()
{
  return 0;
}
