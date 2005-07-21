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
#include "assert.h"

//---------------------------------------------------------------------------
Dentry::Dentry(Dentry *parent)
{
  d_parent_ = parent;
  d_name_ = new Qstr();
  d_inode_ = 0;
  d_count_ = 0;
}

//---------------------------------------------------------------------------
Dentry::~Dentry()
{
  assert(d_count_ != 0);
  assert(d_inode_ != 0);

  delete d_name_;
}

//---------------------------------------------------------------------------
int32 Dentry::dentry_destantiate()
{
  if(d_count_ != 1)
    return -1;
  
  d_inode_ = 0;
  d_count_--;
  return 0;
}
//---------------------------------------------------------------------------
void Dentry::d_child_insert(Dentry *child_dentry)
{
  assert(child_dentry != 0);
  d_count_++;
  d_child_.push_end(child_dentry);
}

//---------------------------------------------------------------------------
int32 Dentry::d_child_remove(Dentry *child_dentry)
{
  assert(child_dentry != 0);
  if(d_child_.remove(child_dentry) == 0)
  {
    d_count_--;
    return 0;
  }
  return -1;
}

//---------------------------------------------------------------------------
bool Dentry::check_name(Dentry *checked_dentry)
{
  bool include = false;
  char* checked_name = checked_dentry->get_name();
  uint32 checked_length = checked_dentry->get_name_length();
  for(uint32 count = 0; count < (d_child_.getLength()); count++)
  {
    Dentry *dentry = (Dentry*)(d_child_.at(count));
    char *tmp_name = dentry->get_name();
    uint32 tmp_length = dentry->get_name_length();
    if(checked_length == tmp_length)
    {
      uint32 index = 0;
      for(; index < checked_length; index++)
      {
        if(checked_name[index] != tmp_name[index])
          break;
      }
      if(index == checked_length)
      {
        include = true;
        break;
      }
    }
  }

  return include;
}

//---------------------------------------------------------------------------
void Dentry::d_delete()
{
  if(d_count_ == 1 && d_inode_ != 0)
  {
    d_parent_->d_child_remove(this);
    //TODO: update in the super_block
  }
}
