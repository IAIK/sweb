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
 * $Id: RamFsFile.cpp,v 1.4 2005/08/04 17:04:00 lythien Exp $
 * $Log: RamFsFile.cpp,v $
 * Revision 1.3  2005/08/02 18:57:57  qiangchen
 * *** empty log message ***
 *
 * Revision 1.2  2005/08/02 18:25:51  lythien
 * write RamFsFile
 *
 */


#include "fs/ramfs/RamFsFile.h"
#include "fs/Dentry.h"
#include "fs/ramfs/RamFsInode.h"

//-----------------------------------------------------------------
// The Constructor
//-----------------------------------------------------------------
RamFsFile::RamFsFile()
{
}

//-----------------------------------------------------------------
// The Destructor
//-----------------------------------------------------------------
RamFsFile::~RamFsFile()
{
}

//----------------------------------------------------------------
// get name of the file
//----------------------------------------------------------------
char *RamFsFile::getName() const
{
  return(f_dentry_->get_name());
}


//----------------------------------------------------------------
// get the dentry
//----------------------------------------------------------------
Dentry *RamFsFile::getDentry() const
{
  return(f_dentry_);
}

//----------------------------------------------------------------
// read from the file
//----------------------------------------------------------------
int32 RamFsFile::read(int32 *buffer, size_t count, l_off_t offset)
{
  return(f_inode_->readData(offset, count, buffer));
}

//----------------------------------------------------------------
// write to the file
//----------------------------------------------------------------
int32 RamFsFile::write(int32 *buffer, size_t count, l_off_t offset)
{
  return(f_inode_->writeData(offset, count, buffer));
}

//----------------------------------------------------------------
// open the file
//----------------------------------------------------------------
int32 RamFsFile::open(Inode*)
{
  return 0;
}

//----------------------------------------------------------------
// close the file
//----------------------------------------------------------------
int32 RamFsFile::close(Inode*)
{
  return 0;
}

//----------------------------------------------------------------
// flush all off the file's wirte operations.
//----------------------------------------------------------------
int32 RamFsFile::flush()
{
  return 0;
}
