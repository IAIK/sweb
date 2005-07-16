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

  // uint8 SEEK_SET = 0;
  // uint8 SEEK_CUR = 1;
  // uint8 SEEK_END = 2;
  // inode_ = inode;
  // return;
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

void File::setName(const char *name)
{

}

//----------------------------------------------------------------
// get name of the file
//----------------------------------------------------------------

const char * File::getName()
{
}


//----------------------------------------------------------------
// get the dentry
//----------------------------------------------------------------

void Dentry::Dentry *getDentry()
{
}

//----------------------------------------------------------------
// read from the file
//----------------------------------------------------------------

void File::read(char *buffer, size_t count, l_off_t offset)
{
}

//----------------------------------------------------------------
// write to the file
//----------------------------------------------------------------

void File::write(char *buffer, size_t count, l_off_t offset)
{
}

//----------------------------------------------------------------
// open the file
//----------------------------------------------------------------

void File::open(Inode* inode)
{
}

//----------------------------------------------------------------
// close the file
//----------------------------------------------------------------

void File::close(Inode* inode)
{
}

//----------------------------------------------------------------
// flush all off the file's wirte operations.
//----------------------------------------------------------------

void File::flush()
{
}
