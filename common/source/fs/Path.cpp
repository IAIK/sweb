#include "Path.h"

Path::Path(Dentry* dentry, VfsMount* mnt) :
    dentry_(dentry), mnt_(mnt)
{

}


bool Path::operator==(const Path& other) const
{
    return ((dentry_ == other.dentry_) && (mnt_ == other.mnt_));
}
