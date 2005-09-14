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
  kprintfd("Dentry::Dentry(char* name)\n");
  this->setName(name);
  d_parent_ = this; // the parent of the root
  d_inode_ = 0;
}

//---------------------------------------------------------------------------
Dentry::Dentry(Dentry *parent)
{
  kprintfd("Dentry::Dentry(Dentry *parent)\n");
  d_parent_ = parent;
  parent->setChild(this);
  d_inode_ = 0;
}

//---------------------------------------------------------------------------
Dentry::~Dentry()
{
  if(d_name_)
    kfree(d_name_);
  kprintfd("~Dentry()\n");
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
  uint32 name_len = strlen(name) + 1;
  d_name_ = (char*)kmalloc(name_len * sizeof(char));

  strlcpy(d_name_, name, name_len);
  kprintfd("\n\n\nd_name_ = %s, has length %d\n", d_name_, strlen(d_name_));
}

//---------------------------------------------------------------------------
char* Dentry::getName() 
{ 
  return d_name_; 
}


//---------------------------------------------------------------------------
int32 Dentry::setChild(Dentry *dentry)
{
  if(dentry == 0)
    return -1;

  if(d_child_.included(dentry) == true)
    return -1;
  
  d_child_.pushBack(dentry);
  
  return 0;
}

Dentry* Dentry::getChild(uint32 index)
{
  assert(index < d_child_.getLength());
  return(d_child_.at(index));
}
//---------------------------------------------------------------------------
Dentry* Dentry::checkName(const char* name)
{
  kprintfd("start of Dentry::checkName\n");
  kprintfd("length of the child: %d\n", d_child_.getLength());
  for(uint32 count = 0; count < (d_child_.getLength()); count++)
  {
    Dentry *dentry = (Dentry*)(d_child_[count]);
    const char *tmp_name = dentry->getName();
    if(strcmp(tmp_name, name) == 0)
    {
      kprintfd("found and end of Dentry::checkName\n");
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
