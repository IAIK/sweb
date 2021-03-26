#include "Path.h"
#include "kprintf.h"
#include "debug.h"
#include "Dentry.h"
#include "VfsMount.h"

Path::Path(Dentry* dentry, VfsMount* mnt) :
    dentry_(dentry), mnt_(mnt)
{

}


bool Path::operator==(const Path& other) const
{
    return ((dentry_ == other.dentry_) && (mnt_ == other.mnt_));
}


Path Path::parentDir(const Path* global_root) const
{
    if(isGlobalRoot(global_root))
    {
        debug(PATHWALKER, "Reached global file system root\n");
        return *this;
    }

    if(dentry_->getParent() == dentry_)
    {
        debug(PATHWALKER, "File system mount root reached, going up a mount to vfsmount %p, mountpoint %p %s\n", mnt_->getParent(), mnt_->getMountPoint(), mnt_->getMountPoint()->getName());

        return Path(mnt_->getMountPoint()->getParent(), mnt_->getParent());
    }

    return Path(dentry_->getParent(), mnt_);
}


ustl::string Path::getAbsolutePath(const Path* global_root) const
{
    if(isGlobalRoot(global_root))
    {
        return "/";
    }
    else if(isMountRoot()) // If this is a mount root, we want the name of the mountpoint instead
    {
        return parentDir().getAbsolutePath() + mnt_->getMountPoint()->getName();
    }
    else if(parentDir().isGlobalRoot()) // Avoid adding '/' twice
    {
        return parentDir().getAbsolutePath() + dentry_->getName();
    }
    else
    {
        return parentDir().getAbsolutePath() + "/" + dentry_->getName();
    }
}

bool Path::isGlobalRoot(const Path* global_root) const
{
    return (global_root && (*this == *global_root)) ||
        (isMountRoot() && mnt_->isRootMount());
}

bool Path::isMountRoot() const
{
    return dentry_->getParent() == dentry_;
}
