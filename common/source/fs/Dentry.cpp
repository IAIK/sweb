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
  kprintfd( "created Dentry with Name %s\n", name);
}

//---------------------------------------------------------------------------
Dentry::Dentry(Dentry *parent)
{
  d_parent_ = parent;
  parent->setChild(this);
  d_inode_ = 0;
  this->setName("NamELLEss");
}

//---------------------------------------------------------------------------
Dentry::~Dentry()
{
  kprintfd( "deleting Dentry with Name %s, d_parent_: %d, this: %d\n", d_name_,d_parent_,this);
  if( d_parent_ && (d_parent_ != this))
  {
    kprintfd( "deleting Dentry child remove d_parent_: %d\n",d_parent_);
    d_parent_->childRemove( this );
  }
  for(uint32 count = 0; count < (d_child_.getLength()); count++)
  {
    Dentry *dentry = (Dentry*)(d_child_.at(count));
    dentry->d_parent_ = 0;
  }
  if(d_name_)
  {
    kprintfd( "deleting Dentry name\n");
    kfree(d_name_);
  }
  kprintfd( "deleting Dentry finished\n");
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
  kprintfd( "Dentry childRemove entering\n");
  kprintfd( "Dentry childRemove d_child_ length: %d \n",d_child_.getLength());
  
  kprintfd( "Dentry childRemove d_child_ included: %d\n",d_child_.included(child_dentry));
  assert(child_dentry != 0);
  if(d_child_.remove(child_dentry) == 0)
  {
    child_dentry->d_parent_ = 0;
    kprintfd( "Dentry childRemove remove == 0\n");
    return 0;
  }

  kprintfd( "Dentry childRemove failed\n");
  return -1;
}

//---------------------------------------------------------------------------
void Dentry::setName(const char* name)
{
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
    Dentry *dentry = (Dentry*)(d_child_.at(count));
    const char *tmp_name = dentry->getName();
    kprintfd("(checkname) name : %s\n",tmp_name);
    if(strcmp(tmp_name, name) == 0)
    {
      return dentry;
    }
  }

  return((Dentry*)0);
}
