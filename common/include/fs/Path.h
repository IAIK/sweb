#pragma once

#include "types.h"

#include "EASTL/string.h"

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

    Path parent(const Path* global_root = nullptr) const;
    int child(const eastl::string& name, Path& out) const;

    eastl::string getAbsolutePath(const Path* global_root = nullptr) const;

    bool isGlobalRoot(const Path* global_root = nullptr) const;
    bool isMountRoot() const;

    Dentry* dentry_;
    VfsMount* mnt_;
};
