// Projectname: SWEB
// Simple operating system for educational purposes
//
// Copyright (C) 2005  Chen Qiang
// Copyright (C) 2005  David Riebenbauer
// Copyright (C) 2005  Maria Mauerhofer
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


#include "fs/File.h"
#include "fs/Dentry.h"
#include "fs/Inode.h"

//-----------------------------------------------------------------
// The Constructor
//-----------------------------------------------------------------

File::File()
{


}


//-----------------------------------------------------------------
// The Destructor
//-----------------------------------------------------------------

File::~File()
{
}

//----------------------------------------------------------------
// set the name of the file
//----------------------------------------------------------------

// int32 File::setName(const char *)
// {
//   assert(0);
//   return 0;
// }


//----------------------------------------------------------------
// get name of the file
//----------------------------------------------------------------

const char *File::getName() const
{

  return(f_dentry_->get_name());

}


//----------------------------------------------------------------
// get the dentry
//----------------------------------------------------------------
const Dentry *File::getDentry() const
{
  return(f_dentry_);

}

//----------------------------------------------------------------
// read from the file
//----------------------------------------------------------------


int32 File::read(int32 *buffer, size_t count, l_off_t offset)
{
//  return(f_inode_ ->readData(offset, count, buffer));

}

//----------------------------------------------------------------
// write to the file
//----------------------------------------------------------------


int32 File::write(int32 *buffer, size_t count, l_off_t offset)
{

  // isn't ready now..
  // return(f_inode_->writeData(offset, count, buffer));

  return 0;

}

//----------------------------------------------------------------
// open the file
//----------------------------------------------------------------

int32 File::open(Inode*)
{
  return 0;

}

//----------------------------------------------------------------
// close the file
//----------------------------------------------------------------

int32 File::close(Inode*)
{
  return 0;
}

//----------------------------------------------------------------
// flush all off the file's wirte operations.
//----------------------------------------------------------------

int32 File::flush()
{
  return 0;
}
