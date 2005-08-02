// Projectname: SWEB
// Simple operating system for educational purposes
//
// Copyright (C) 2004 Maria Mauerhofer
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
 * $Id: RamFsFile.cpp,v 1.2 2005/08/02 18:25:51 lythien Exp $
 * $Log$
 */


#include "fs/ramfs/RamFsFile.h"
#include "fs/ramfs/RamFsDentry.h"
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

virtual char *RamFsFile::getName() const
{

  return(f_dentry_->get_name());

}


//----------------------------------------------------------------
// get the dentry
//----------------------------------------------------------------
virtual Dentry *RamFsFile::getDentry() const
{
  return(f_dentry_);
}

//----------------------------------------------------------------
// read from the file
//----------------------------------------------------------------


virtual int32 RamFsFile::read(int32 *buffer, size_t count, l_off_t offset)
{
  return(f_inode_ ->readData(offset, count, buffer));
}

//----------------------------------------------------------------
// write to the file
//----------------------------------------------------------------


virtual int32 RamFsFile::write(int32 *buffer, size_t count, l_off_t offset)
{
  //return(f_inode_->writeData(offset, count, buffer));
}

//----------------------------------------------------------------
// open the file
//----------------------------------------------------------------

virtual int32 RamFsFile::open(Inode*)
{
  return 0;
}

//----------------------------------------------------------------
// close the file
//----------------------------------------------------------------

virtual int32 RamFsFile::close(Inode*)
{
  return 0;
}

//----------------------------------------------------------------
// flush all off the file's wirte operations.
//----------------------------------------------------------------

virtual int32 RamFsFile::flush()
{
  return 0;
}
