
//
// CVS Log Info for $RCSfile: FileSystemType.cpp,v $
//
// $Id: FileSystemType.cpp,v 1.1 2005/07/19 17:11:03 davrieb Exp $
// $Log$
//
//

#include "fs/FileSystemType.h"
#include "assert.h"

//----------------------------------------------------------------------
FileSystemType::FileSystemType() :
  fs_name_(0),
  fs_flags_(0)
{
}

//----------------------------------------------------------------------
FileSystemType::~FileSystemType()
{
}

//----------------------------------------------------------------------
const char* FileSystemType::getFSName() const
{
  return fs_name_;
}

//----------------------------------------------------------------------
int32 FileSystemType::getFSFlags() const
{
  return fs_flags_;
}

//----------------------------------------------------------------------
Superblock *FileSystemType::readSuper(Superblock *superblock, void *data)
{
  assert(0);
  return (0);
}

