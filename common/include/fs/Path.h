#pragma once

#include "types.h"
#include "ustring.h"

class Dentry;
class VfsMount;

class Path
{
public:
    Path() = default;
    Path(Dentry* dentry, VfsMount* mnt);
    Path(const Path&) = default;
    Path& operator=(const Path&) = default;
    bool operator==(const Path&) const;

    Path parentDir(const Path* global_root = nullptr) const;
    ustl::string getAbsolutePath(const Path* global_root = nullptr) const;

    bool isGlobalRoot(const Path* global_root = nullptr) const;
    bool isMountRoot() const;

    Dentry* dentry_;
    VfsMount* mnt_;
};
