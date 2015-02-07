#include "Dentry.h"
#include "assert.h"
#include "Inode.h"

#include "kprintf.h"

Dentry::Dentry(const char* name) :
    d_inode_(0), d_parent_(this), d_mounts_(0), d_name_(name)
{
  debug(DENTRY, "created Dentry with Name %s\n", name);
}

Dentry::Dentry(Dentry *parent) :
    d_inode_(0), d_parent_(parent), d_mounts_(0), d_name_("NamELLEss")
{
  parent->setChild(this);
}

Dentry::~Dentry()
{
  debug(DENTRY, "deleting Dentry with Name %s, d_parent_: %p, this: %p\n", d_name_.c_str(), d_parent_, this);
  if (d_parent_ && (d_parent_ != this))
  {
    debug(DENTRY, "deleting Dentry child remove d_parent_: %p\n", d_parent_);
    d_parent_->childRemove(this);
  }
  for (Dentry* dentry : d_child_)
    dentry->d_parent_ = 0;
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
  debug(DENTRY, "Dentry childRemove d_child_ included: %d\n",
        ustl::find(d_child_.begin(), d_child_.end(), child_dentry) != d_child_.end());
  assert(child_dentry != 0);
  d_child_.remove(child_dentry);
  child_dentry->d_parent_ = 0;
  debug(DENTRY, "Dentry childRemove remove == 0\n");
  return 0;
}

const char* Dentry::getName()
{
  return d_name_.c_str();
}

int32 Dentry::setChild(Dentry *dentry)
{
  if (dentry == 0 || ustl::find(d_child_.begin(), d_child_.end(), dentry) != d_child_.end())
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
      return dentry;
  }

  return 0;
}

uint32 Dentry::getNumChild()
{
  return d_child_.size();
}

bool Dentry::emptyChild()
{
  return d_child_.empty();
}
