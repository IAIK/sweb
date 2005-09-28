// Projectname: SWEB
// Simple operating system for educational purposes

#include "fs/Dentry.h"
#include "util/string.h"
#include "assert.h"
#include "mm/kmalloc.h"
#include "fs/Inode.h"

#define STRLCOPY_ERR "strlcpy error"

#include "console/kprintf.h"
//---------------------------------------------------------------------------
Dentry::Dentry(const char* name)
{
  this->setName(name);
  d_parent_ = this; // the parent of the root
  d_inode_ = 0;
}

//---------------------------------------------------------------------------
Dentry::Dentry(Dentry *parent)
{
  d_parent_ = parent;
  parent->setChild(this);
  d_inode_ = 0;
}

//---------------------------------------------------------------------------
Dentry::~Dentry()
{
  kprintfd("remove the dentry: %s\n", d_name_);
  if(d_name_)
    kfree(d_name_);
}

//---------------------------------------------------------------------------
void Dentry::setInode(Inode *inode)
{ 
  d_inode_ = inode; 
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
void Dentry::setName(const char* name)
{
  kprintfd("set name: %s\n", name);
  uint32 name_len = strlen(name) + 1;
  d_name_ = (char*)kmalloc(name_len * sizeof(char));

  strlcpy(d_name_, name, name_len);
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

//---------------------------------------------------------------------------
Dentry* Dentry::getChild(uint32 index)
{
  assert(index < d_child_.getLength());
  return(d_child_.at(index));
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
