#pragma once

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

    Dentry* dentry_;
    VfsMount* mnt_;
};
