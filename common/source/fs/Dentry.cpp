#include "Dentry.h"

#include "Inode.h"
#include "kprintf.h"

#include "assert.h"

Dentry::Dentry(Inode* inode) :
    d_inode_(inode), d_parent_(this), d_mounts_(0), d_name_("/")
{
    debug(DENTRY, "Created root Dentry\n");
    inode->addDentry(this);
}

Dentry::Dentry(Inode* inode, Dentry* parent, const eastl::string& name) :
    d_inode_(inode), d_parent_(parent), d_mounts_(0), d_name_(name)
{
    debug(DENTRY, "created Dentry with Name %s\n", name.c_str());
    assert(name != "");
    parent->setChild(this);
    inode->addDentry(this);
}

Dentry::~Dentry()
{
  debug(DENTRY, "Deleting Dentry %s, d_parent_: %p, this: %p\n", d_name_.c_str(), d_parent_, this);
  if (d_parent_ && (d_parent_ != this))
  {
    d_parent_->childRemove(this);
  }
  for (Dentry* dentry : d_child_)
    dentry->d_parent_ = nullptr;

  d_inode_->removeDentry(this);
}

void Dentry::setInode(Inode *inode)
{
  d_inode_ = inode;
}

void Dentry::childInsert(Dentry *child_dentry)
{
  assert(child_dentry != nullptr);
  d_child_.push_back(child_dentry);
}

int32 Dentry::childRemove(Dentry *child_dentry)
{
  debug(DENTRY, "Dentry childRemove d_child_ included: %d\n",
        eastl::find(d_child_.begin(), d_child_.end(), child_dentry) != d_child_.end());
  assert(child_dentry != nullptr);
  assert(child_dentry->d_parent_ == this);
  d_child_.remove(child_dentry);
  child_dentry->d_parent_ = nullptr;
  return 0;
}

const char* Dentry::getName() const
{
  return d_name_.c_str();
}

int32 Dentry::setChild(Dentry *dentry)
{
  if (dentry == nullptr || eastl::find(d_child_.begin(), d_child_.end(), dentry) != d_child_.end())
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

  return nullptr;
}

uint32 Dentry::getNumChild()
{
  return d_child_.size();
}

bool Dentry::emptyChild()
{
    return d_child_.empty();
}

bool Dentry::emptyChild(std::initializer_list<const char*> exceptDentries)
{
    for (auto child : d_child_)
    {
        for (auto excludedName : exceptDentries)
        {
            if (strcmp(child->getName(), excludedName) == 0)
            {
                continue;
            }
        }

        return false;
    }

    return true;
}
