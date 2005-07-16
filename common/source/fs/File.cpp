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
 //  inode_ = inode;
//   return;
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

File::setName(const char *name)
{
}

//----------------------------------------------------------------
// get name of the file
//----------------------------------------------------------------

File::const char *getName()
{
}


//----------------------------------------------------------------
// get the dentry
//----------------------------------------------------------------

Dentry::Dentry *getDentry()
{
}

//----------------------------------------------------------------
// read from the file
//----------------------------------------------------------------

File::read(char *buffer, size_t count, l_off_t offset)
{
}

//----------------------------------------------------------------
// write to the file
//----------------------------------------------------------------

File::write(char *buffer, size_t count, l_off_t offset)
{
}

//----------------------------------------------------------------
// open the file
//----------------------------------------------------------------

File::open(Inode* inode)
{
}

//----------------------------------------------------------------
// close the file
//----------------------------------------------------------------

File::close()
{
}

//----------------------------------------------------------------
// flush all off the file's wirte operations.
//----------------------------------------------------------------

File::flush()
{
}
