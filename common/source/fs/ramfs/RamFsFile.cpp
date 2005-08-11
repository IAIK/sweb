// Projectname: SWEB
// Simple operating system for educational purposes
//
// Copyright (C) 2005 Maria Mauerhofer
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.



/**
 * CVS Log Info for $RCSfile: RamFsFile.cpp,v $
 *
 * $Id: RamFsFile.cpp,v 1.5 2005/08/11 16:35:59 qiangchen Exp $
 * $Log: RamFsFile.cpp,v $
 * Revision 1.4  2005/08/04 17:04:00  lythien
 * include the methode write
 *
 * Revision 1.3  2005/08/02 18:57:57  qiangchen
 * *** empty log message ***
 *
 * Revision 1.2  2005/08/02 18:25:51  lythien
 * write RamFsFile
 *
 */

#include "fs/ramfs/RamFsInode.h"
#include "fs/ramfs/RamFsFile.h"
#include "fs/ramfs/RamFsSuperblock.h"
#include "fs/Dentry.h"

#define ERROR_FRO "ERROR: The flag muss be READONLY for several opened files"
#define ERROR_FF  "ERROR: The flag does not allow this operation"
#define ERROR_FNO "ERROR: The file is not open."

//--------------------------------------------------------------------------
RamFsFile::RamFsFile(Inode* inode, Dentry* dentry) : File(inode, dentry)
{
  f_superblock_ = inode->getSuperblock();
  count_ = 0;
  mode_ = (A_READABLE ^ A_WRITABLE) ^ A_EXECABLE;
  offset_ = 0;
}

//--------------------------------------------------------------------------
RamFsFile::~RamFsFile()
{
  assert(count_ != 0);
}

//--------------------------------------------------------------------------
char *RamFsFile::getName() const
{
  return(f_dentry_->get_name());
}

//--------------------------------------------------------------------------
Dentry *RamFsFile::getDentry() const
{
  return(f_dentry_);
}

//--------------------------------------------------------------------------
int32 RamFsFile::read(int32 *buffer, size_t count, l_off_t offset)
{
  if(f_superblock_->check_opened_files(this) == false)
  {
    // ERROR_FNO
    return -1;
  }
  
  if((flag_ == O_RDONLY) || (flag_ == O_RDWR))
    return(f_inode_->readData(offset, count, buffer));
  else
  {
    // ERROR_FF
    return -1;
  }   
}

//--------------------------------------------------------------------------
int32 RamFsFile::write(int32 *buffer, size_t count, l_off_t offset)
{
  if(f_superblock_->check_opened_files(this) == false)
  {
    // ERROR_FNO
    return -1;
  }

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

  if(f_inode_->is_opened_files_empty() == true)
  {
  }
  else if((f_inode_->insert_opened_files(this) == 0) && (flag == O_RDONLY))
  {
  }
  else
  {
    // ERROR_FRO
    return -1;
  }
  
  f_inode_->insert_opened_files(this);
  flag_ = flag;
  f_superblock_->insert_opened_files(this);

  return 0;
}

//--------------------------------------------------------------------------
int32 RamFsFile::close()
{
  assert((f_inode_->remove_opened_files(this) != 0) &&
         (f_superblock_->remove_opened_files(this) != 0));
}

//--------------------------------------------------------------------------
int32 RamFsFile::flush()
{
  return 0;
}
