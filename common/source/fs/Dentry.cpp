/**
 * @file DEntry.cpp
 */

#include "fs/Dentry.h"
#include "kstring.h"
#include "assert.h"
#include "mm/kmalloc.h"
#include "fs/Inode.h"
#include <ustl/ualgo.h>

#include "console/kprintf.h"
#include "console/debug.h"

Dentry::Dentry(const char* name) :
    d_name_(0)
{
  this->setName(name);
  d_parent_ = this; // the parent of the root
  d_inode_ = 0;
  debug(DENTRY, "created Dentry with Name %s\n", name);
}

Dentry::Dentry(Dentry *parent) :
    d_name_(0)
{
  d_parent_ = parent;
  parent->setChild(this);
  d_inode_ = 0;
  this->setName("NamELLEss");
}

Dentry::~Dentry()
{
  debug(DENTRY, "deleting Dentry with Name %s, d_parent_: %d, this: %d\n", d_name_, d_parent_, this);
  if (d_parent_ && (d_parent_ != this))
  {
    debug(DENTRY, "deleting Dentry child remove d_parent_: %d\n", d_parent_);
    d_parent_->childRemove(this);
  }
  for (Dentry* dentry : d_child_)
  {
    dentry->d_parent_ = 0;
  }
  if (d_name_)
  {
    debug(DENTRY, "deleting Dentry name\n");
    delete[] d_name_;
  }
  debug(DENTRY, "deleting Dentry finished\n");
}

void Dentry::setInode(Inode *inode)
{
  d_inode_ = inode;
}

void Dentry::childInsert(Dentry *child_dentry)
{
  assert(child_dentry != 0);
  d_child_.push_back(child_dentry);
}

int32 Dentry::childRemove(Dentry *child_dentry)
{
  debug(DENTRY, "Dentry childRemove entering\n");
  debug(DENTRY, "Dentry childRemove d_child_ length: %d \n", d_child_.size());

  debug(DENTRY, "Dentry childRemove d_child_ included: %d\n",
        ustl::find(d_child_.begin(), d_child_.end(), child_dentry ) != d_child_.end());
  assert(child_dentry != 0);
  d_child_.remove(child_dentry);
  child_dentry->d_parent_ = 0;
  debug(DENTRY, "Dentry childRemove remove == 0\n");
  return 0;
}

void Dentry::setName(const char* name)
{
  delete[] d_name_;
  uint32 name_len = strlen(name) + 1;
  d_name_ = new char[name_len * sizeof(char)];

  strncpy(d_name_, name, name_len);
  d_name_[name_len - 1] = 0;
}

char* Dentry::getName()
{
  return d_name_;
}

int32 Dentry::setChild(Dentry *dentry)
{
  if (dentry == 0)
    return -1;

  if (ustl::find(d_child_.begin(), d_child_.end(), dentry) != d_child_.end())
    return -1;

  d_child_.push_back(dentry);

  return 0;
}

Dentry* Dentry::checkName(const char* name)
{
  for (Dentry* dentry : d_child_)
  {
    const char *tmp_name = dentry->getName();
    debug(DENTRY, "(checkname) name : %s\n", tmp_name);
    if (strcmp(tmp_name, name) == 0)
    {
      return dentry;
    }
  }

  return 0;
}
