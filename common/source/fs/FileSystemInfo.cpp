#include "FileSystemInfo.h"
#include "Dentry.h"
#include "kstring.h"
#include "assert.h"

FileSystemInfo::FileSystemInfo() :
    root_(0), root_mnt_(0), pwd_(0), pwd_mnt_(0), alt_root_(0), alt_root_mnt_(0)
{
}

FileSystemInfo::FileSystemInfo(const FileSystemInfo& fsi) :
    root_(fsi.root_), root_mnt_(fsi.root_mnt_), pwd_(fsi.pwd_), pwd_mnt_(fsi.pwd_mnt_), alt_root_(fsi.alt_root_),
    alt_root_mnt_(0)
{
}

FileSystemInfo::~FileSystemInfo()
{
}
