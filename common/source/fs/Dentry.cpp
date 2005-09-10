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

#include "fs/Dentry.h"
#include "util/string.h"
#include "assert.h"
#include "mm/kmalloc.h"

#define STRLCOPY_ERR "strlcpy error"

//---------------------------------------------------------------------------
Dentry::Dentry(char* name)
{
  this->setName(name);
  d_parent_ = this; // the parent of the root
  d_inode_ = 0;
}

//---------------------------------------------------------------------------
Dentry::Dentry(Dentry *parent)
{
  d_parent_ = parent;
  d_inode_ = 0;
}

//---------------------------------------------------------------------------
Dentry::~Dentry()
{
  assert(d_inode_ != 0);

  if(d_name_)
    kfree(d_name_);
}

//---------------------------------------------------------------------------
void Dentry::childInsert(Dentry *child_dentry)
{
  assert(child_dentry != 0);
  d_child_.pushBack(child_dentry);
}

//---------------------------------------------------------------------------
int32 Dentry::childRemove(Dentry *child_dentry)
{
  assert(child_dentry != 0);
  if(d_child_.remove(child_dentry) == 0)
    return 0;

  return -1;
}

//---------------------------------------------------------------------------
void Dentry::setName(char* name)
{
  uint32 name_len = strlen(name);
  d_name_ = (char*)kmalloc(name_len * sizeof(char));
  
  int32 copied = strlcpy(d_name_, name, name_len);
  if(copied >= ((int32)name_len))
  {
    // STRLCOPY_ERR
    kfree(d_name_);
    d_name_ = 0;
  } 
}

//---------------------------------------------------------------------------
char* Dentry::getName() 
{ 
  return d_name_; 
}

//---------------------------------------------------------------------------
Dentry* Dentry::checkName(const char* name)
{
  for(uint32 count = 0; count < (d_child_.getLength()); count++)
  {
    Dentry *dentry = (Dentry*)(d_child_[count]);
    const char *tmp_name = dentry->getName();
    if(strcmp(tmp_name, name) == 0)
    {
      return dentry;
    }
  }

  return((Dentry*)0);
}

//---------------------------------------------------------------------------
// void Dentry::d_delete()
// {
  
//   if(d_count_ == 1 && d_inode_ != 0)
//   {
//     d_parent_->d_child_remove(this);
//   }
// }
