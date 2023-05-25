#include "FileSystemInfo.h"

FileSystemInfo::FileSystemInfo() :
    root_(), pwd_()
{
}

FileSystemInfo::FileSystemInfo(const FileSystemInfo& fsi) :
    root_(fsi.root_), pwd_(fsi.pwd_)
{
}
