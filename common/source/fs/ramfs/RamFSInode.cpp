/**
 * @file RamFSInode.cpp
 */

#include "fs/ramfs/RamFSInode.h"

#include "mm/kmalloc.h"
#include "util/string.h"
#include "assert.h"
#include "fs/ramfs/RamFSSuperblock.h"
#include "fs/ramfs/RamFSFile.h"
#include "fs/Dentry.h"

#include "console/kprintf.h"

#define BASIC_ALLOC 256
#define ERROR_DNE "Error: the dentry does not exist.\n"
#define ERROR_DU  "Error: inode is used.\n"
#define ERROR_IC  "Error: invalid command (only for Directory).\n"
#define ERROR_NNE "Error: the name does not exist in the current directory.\n"
#define ERROR_HLI "Error: hard link invalid.\n"
#define ERROR_DNEILL "Error: the dentry does not exist in the link list.\n"
#define ERROR_DEC "Error: the dentry exists child.\n"


RamFSInode::RamFSInode ( Superblock *super_block, uint32 inode_type ) :
    Inode ( super_block, inode_type )
{
  if ( inode_type == I_FILE )
    data_ = ( char* ) kmalloc ( BASIC_ALLOC );
  else
    data_ = 0;

  i_size_ = BASIC_ALLOC;
  i_nlink_ = 0;
  i_dentry_ = 0;
}


RamFSInode::~RamFSInode()
{
  if ( data_ )
  {
    kfree ( data_ );
  }
}


int32 RamFSInode::readData ( int32 offset, int32 size, char *buffer )
{
  if ( ( size + offset ) > BASIC_ALLOC )
  {
    kprintfd ( "RamFSInode::ERROR: the size is bigger than size of the file\n" );
    assert ( true );
  }

  char *ptr_offset = data_ + offset;
  memcpy ( buffer, ptr_offset, size );
  return size;
}


int32 RamFSInode::writeData ( int32 offset, int32 size, const char *buffer )
{
  if ( ( size + offset ) > BASIC_ALLOC )
  {
    kprintfd ( "RamFSInode::ERROR: the size is bigger than size of the file\n" );
    assert ( true );
  }

  assert ( i_type_ == I_FILE );

  char *ptr_offset = data_ + offset;
  memcpy ( ptr_offset, buffer, size );
  return size;
}


int32 RamFSInode::mknod ( Dentry *dentry )
{
  if ( dentry == 0 )
  {
    // ERROR_DNE
    return -1;
  }

  if ( i_type_ != I_DIR )
  {
    // ERROR_IC
    return -1;
  }

  i_dentry_ = dentry;
  dentry->setInode ( this );
  return 0;
}


int32 RamFSInode::mkdir ( Dentry *dentry )
{
  return ( mkdir ( dentry ) );
}


int32 RamFSInode::mkfile ( Dentry *dentry )
{
  if ( dentry == 0 )
  {
    // ERROR_DNE
    return -1;
  }

  if ( i_type_ != I_FILE )
  {
    // ERROR_IC
    return -1;
  }

  i_dentry_ = dentry;
  dentry->setInode ( this );
  return 0;
}


int32 RamFSInode::create ( Dentry *dentry )
{
  return ( mkdir ( dentry ) );
}


File* RamFSInode::link ( uint32 flag )
{
  File* file = ( File* ) ( new RamFSFile ( this, i_dentry_, flag ) );
  i_files_.pushBack ( file );
  return file;
}


int32 RamFSInode::unlink ( File* file )
{
  int32 tmp = i_files_.remove ( file );
  delete file;
  return tmp;
}


int32 RamFSInode::rmdir()
{
  if ( i_type_ != I_DIR )
    return -1;

  Dentry* dentry = i_dentry_;

  if ( dentry->emptyChild() == true )
  {
    dentry->releaseInode();
    Dentry* parent_dentry = dentry->getParent();
    parent_dentry->childRemove ( dentry );
    delete dentry;
    i_dentry_ = 0;
    return INODE_DEAD;
  }
  else
  {
    // ERROR_DEC
    return -1;
  }
}


int32 RamFSInode::rm()
{
  if ( i_files_.getLength() != 0 )
  {
    kprintfd ( "RamFSInode::ERROR: the file is opened.\n" );
    return -1;
  }

  Dentry* dentry = i_dentry_;

  if ( dentry->emptyChild() == true )
  {
    dentry->releaseInode();
    Dentry* parent_dentry = dentry->getParent();
    parent_dentry->childRemove ( dentry );
    delete dentry;
    i_dentry_ = 0;
    return INODE_DEAD;
  }
  else
  {
    // ERROR_DEC
    return -1;
  }
}


Dentry* RamFSInode::lookup ( const char* name )
{
  if ( name == 0 )
  {
    // ERROR_DNE
    return 0;
  }

  Dentry* dentry_update = 0;
  if ( i_type_ == I_DIR )
  {
    dentry_update = i_dentry_->checkName ( name );
    if ( dentry_update == 0 )
    {
      // ERROR_NNE
      return ( Dentry* ) 0;
    }
    else
    {
      return dentry_update;
    }
  }
  else
  {
    // ERROR_IC
    return ( Dentry* ) 0;
  }
}

