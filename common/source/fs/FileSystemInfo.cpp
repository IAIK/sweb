#include "FileSystemInfo.h"
#include "Dentry.h"
#include "kstring.h"
#include "assert.h"

FileSystemInfo::FileSystemInfo() :
    root_(nullptr), root_mnt_(nullptr), pwd_(nullptr), pwd_mnt_(nullptr), alt_root_(nullptr), alt_root_mnt_(nullptr)
{
}

FileSystemInfo::FileSystemInfo(const FileSystemInfo& fsi) :
    root_(fsi.root_), root_mnt_(fsi.root_mnt_), pwd_(fsi.pwd_), pwd_mnt_(fsi.pwd_mnt_), alt_root_(fsi.alt_root_),
    alt_root_mnt_(nullptr)
{
}
