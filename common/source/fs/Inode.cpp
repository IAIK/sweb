// Projectname: SWEB
// Simple operating system for educational purposes
//
// Copyright (C) 2005  Chen Qiang 
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

#include "fs/Inode.h"
#include "assert.h"

#define ERROR_FNE "Error: the file does not exist."
#define ERROR_FE  "Error: the file is exists."

//---------------------------------------------------------------------------
int32 Inode::insertOpenedFiles(File* file)
{
  if(file == 0)
  {
    // ERROR_FNE
    return -1;
  }
  
  if(i_files_.included(file) == true)
  {
    // ERROR_FE
    return -1;
  }
  i_files_.pushBack(file);
  
  return 0;
}

//---------------------------------------------------------------------------
int32 Inode::removeOpenedFiles(File* file)
{
  if(file == 0)
  {
    // ERROR_FNE
    return -1;
  }
  
  if(i_files_.included(file) == true)
    i_files_.remove(file);
  else
  {
    // ERROR_FNE
    return -1;
  }
  
  return 0;
}

//---------------------------------------------------------------------------
