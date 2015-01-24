/**
 * @file FileSystemType.cpp
 */

#include "fs/FileSystemType.h"
#include "assert.h"

FileSystemType::FileSystemType(const char *fs_name) :
    fs_name_ ( fs_name ),
    fs_flags_ ( 0 )
{}


FileSystemType::~FileSystemType()
{}


const char* FileSystemType::getFSName() const
{
  return fs_name_;
}


int32 FileSystemType::getFSFlags() const
{
  return fs_flags_;
}


Superblock *FileSystemType::readSuper ( Superblock* /*superblock*/, void* /*data*/ ) const
{
  assert ( 0 );
  return ( 0 );
}


Superblock *FileSystemType::createSuper ( Dentry* /*dentry*/, uint32 /*s_dev*/ ) const
{
  assert ( 0 );
  return ( Superblock* ) 0;
}

