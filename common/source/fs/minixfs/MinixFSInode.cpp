
#include "fs/minixfs/MinixFSInode.h"

#include "mm/kmalloc.h"
#include "util/string.h"
#include "assert.h"
#include "fs/minixfs/MinixFSSuperblock.h"
#include "fs/minixfs/MinixFSFile.h"
#include "fs/Dentry.h"
#include "arch_bd_manager.h"

#include "console/kprintf.h"

#define ERROR_DNE "Error: the dentry does not exist.\n"
#define ERROR_DU  "Error: inode is used.\n"
#define ERROR_IC  "Error: invalid command (only for Directory).\n"
#define ERROR_NNE "Error: the name does not exist in the current directory.\n"
#define ERROR_HLI "Error: hard link invalid.\n"
#define ERROR_DNEILL "Error: the dentry does not exist in the link list.\n"
#define ERROR_DEC "Error: the dentry exists child.\n"


#define DENTRY_SIZE 32
#define MAX_NAME_LENGTH 30

//---------------------------------------------------------------------------
MinixFSInode::MinixFSInode(Superblock *super_block, uint32 inode_type) :
    Inode(super_block, inode_type)
{
  i_zones_ = 0;
  i_size_ = 0;
  i_nlink_ = 0;
  i_dentry_ = 0;
}

//---------------------------------------------------------------------------
MinixFSInode::MinixFSInode(Superblock *super_block, uint16 i_mode, uint16 i_uid, uint32 i_size, uint32 i_modtime, uint8 i_gid, uint8 i_nlinks, uint16* i_zones, uint32 i_num) : Inode(super_block, 0)
{
  i_size_ = i_size;
  i_nlink_ = i_nlinks;
  i_dentry_ = 0;
  i_state_ = I_UNUSED;
  i_zones_ = new MinixFSZone((MinixFSSuperblock *)super_block, i_zones);
  i_num_ = i_num;
  if(i_mode & 0x8000)
  {
    i_type_ = I_FILE;
  }
  else if(i_mode & 0x4000)
  {
    i_type_ = I_DIR;
  }
  //TODO: else something else (hard/sym link)

  kprintfd( "created inode with size: %x\tnlink: %x\tzones[0]: %x\tmode: %x\n", i_size_, i_nlink_, i_zones_->getZone(0), i_mode);
}

//---------------------------------------------------------------------------
MinixFSInode::~MinixFSInode()
{
  delete i_zones_;
}

//---------------------------------------------------------------------------
int32 MinixFSInode::readData(uint32 offset, uint32 size, char *buffer)
{
  if((size + offset) > i_size_)
  {
    kprintfd("the size is bigger than size of the file\n");
    assert(false);
  }
  uint32 zone = offset / MINIX_ZONE_SIZE;
  uint32 zone_offset = offset % MINIX_ZONE_SIZE;
  uint32 num_zones = (zone_offset + size) / MINIX_ZONE_SIZE + 1;
  Buffer* rbuffer = new Buffer(MINIX_ZONE_SIZE);
  
  uint32 index = 0;
  
  for(;zone < num_zones; zone++)
  {
    rbuffer->clear();
    ((MinixFSSuperblock *)i_superblock_)->readZone(i_zones_->getZone(zone), rbuffer);
    for(; index < size && (index + zone_offset) < MINIX_ZONE_SIZE; index++)
    {
      buffer[index] = rbuffer->getByte( index + zone_offset);
    }
    zone_offset = 0;
  }
  delete rbuffer;
  return size;
}

//---------------------------------------------------------------------------
int32 MinixFSInode::writeData(uint32 offset, uint32 size, const char *buffer)
{
  uint32 zone = offset / MINIX_ZONE_SIZE;
  uint32 num_zones = (offset % MINIX_ZONE_SIZE + size) / MINIX_ZONE_SIZE + 1;
  uint32 last_used_zone = i_size_/MINIX_ZONE_SIZE;
  uint32 last_zone = last_used_zone;
  
  if((size + offset) > i_size_)
  {
    uint32 num_new_zones = (size + offset - i_size_) / MINIX_ZONE_SIZE + 1;
    for(uint32 new_zones = 0; new_zones < num_new_zones; new_zones++, last_zone++)
    {
      i_zones_->setZone(last_zone + 1, ((MinixFSSuperblock*)i_superblock_)->allocateZone());
    }
  }
  
  if(offset > i_size_)
  {
    uint32 zone_size_offset =  i_size_%MINIX_ZONE_SIZE;
    Buffer* fill_buffer = new Buffer(MINIX_ZONE_SIZE);
    fill_buffer->clear();
    if (zone_size_offset)
    {
      readData( i_size_-zone_size_offset, zone_size_offset, fill_buffer->getBuffer());
      ((MinixFSSuperblock *)i_superblock_)->writeZone( last_used_zone, fill_buffer);
    }
    ++last_used_zone;
    for (; last_used_zone <= offset/MINIX_ZONE_SIZE; last_used_zone++)
    {
      fill_buffer->clear();
      ((MinixFSSuperblock *)i_superblock_)->writeZone( last_used_zone, fill_buffer );
    }
    --last_used_zone;
    i_size_ = offset;
    delete fill_buffer;
  }
  
  uint32 zone_offset = offset%MINIX_ZONE_SIZE;
  Buffer* wbuffer = new Buffer(num_zones * MINIX_ZONE_SIZE);
  readData( offset-zone_offset, zone_offset, wbuffer->getBuffer());
  for(uint32 index = 0, pos = zone_offset; index<size; pos++, index++)
  {
    wbuffer->setByte( pos, buffer[index]);
  }
  for(uint32 zone_index = 0; zone_index < num_zones; zone_index++)
  {
    wbuffer->setOffset(zone_index*MINIX_ZONE_SIZE);
    ((MinixFSSuperblock *)i_superblock_)->writeZone(zone_index + zone, wbuffer);
  }
  if(i_size_ < offset + size)
  {
    i_size_ = offset + size;
  }
  delete wbuffer;
  return size;
}

//---------------------------------------------------------------------------
int32 MinixFSInode::mknod(Dentry *dentry)
{
  if(dentry == 0)
  {
    // ERROR_DNE
    return -1;
  }

  if(i_type_ == I_DIR || i_type_ == I_FILE)
  {
    // ERROR_IC
    return -1;
  }
  //TODO write to disc
  i_dentry_ = dentry;
  dentry->setInode(this);
  return 0;
}

//---------------------------------------------------------------------------
int32 MinixFSInode::mkdir(Dentry *dentry)
{
  if(dentry == 0)
  {
    // ERROR_DNE
    return -1;
  }

  if(i_type_ != I_DIR)
  {
    // ERROR_IC
    return -1;
  }
  //TODO write to disc

  i_dentry_ = dentry;
  dentry->setInode(this);
  return 0;
}

//---------------------------------------------------------------------------
int32 MinixFSInode::mkfile(Dentry *dentry)
{
  if(dentry == 0)
  {
    // ERROR_DNE
    return -1;
  }

  if(i_type_ != I_FILE)
  {
    // ERROR_IC
    return -1;
  }

  //TODO write to disc
  i_dentry_ = dentry;
  dentry->setInode(this);
  return 0;
}

//---------------------------------------------------------------------------
int32 MinixFSInode::create(Dentry *dentry)
{
  //TODO implement
  assert(false);
  return(mkdir(dentry));
}

//---------------------------------------------------------------------------
File* MinixFSInode::link(uint32 flag)
{
  File* file = (File*)(new MinixFSFile(this, i_dentry_, flag));
  i_files_.pushBack(file);
  ++i_nlink_;
  return file;
}

//---------------------------------------------------------------------------
int32 MinixFSInode::unlink(File* file)
{
  int32 tmp = i_files_.remove(file);
  delete file;
  --i_nlink_;
  return tmp;
}

//---------------------------------------------------------------------------
int32 MinixFSInode::rmdir()
{
  if(i_type_ != I_DIR)
    return -1;

  Dentry* dentry = i_dentry_;

  //NOTE empty instead of emptyChild?
  if(dentry->emptyChild() == true)
  {
    dentry->releaseInode();
    Dentry* parent_dentry = dentry->getParent();
    parent_dentry->childRemove(dentry);
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

//---------------------------------------------------------------------------
int32 MinixFSInode::rm()
{
  if(i_files_.getLength() != 0)
  {
    kprintfd("the file is opened.\n");
    return -1;
  }

  Dentry* dentry = i_dentry_;

  //NOTE empty instead of emptyChild?
  //NOTE has to be file so should not have children
  if(dentry->emptyChild() == true)
  {
    dentry->releaseInode();
    Dentry* parent_dentry = dentry->getParent();
  //NOTE if it is possible to be a directory it has to be deleted recursively
    parent_dentry->childRemove(dentry);
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

//---------------------------------------------------------------------------
Dentry* MinixFSInode::lookup(const char* name)
{
  if(name == 0)
  {
    // ERROR_DNE
    return 0;
  }

  Dentry* dentry_update = 0;
  if(i_type_ == I_DIR)
  {
    dentry_update = i_dentry_->checkName(name);
    if(dentry_update == 0)
    {
      // ERROR_NNE
      return (Dentry*)0;
    }
    else
    {
      ((MinixFSInode *)dentry_update->getInode())->loadChildren();
      return dentry_update;
    }
  }
  else
  {
    // ERROR_IC
    return (Dentry*)0;
  }
}

void MinixFSInode::loadChildren()
{
  Buffer *dbuffer = new Buffer(MINIX_ZONE_SIZE);
  for (uint32 zone = 0; zone < i_zones_->getNumZones(); zone++)
  {
    kprintfd("calling readZone with the zone: %d\n", i_zones_->getZone(zone));
    ((MinixFSSuperblock *)i_superblock_)->readZone(i_zones_->getZone(zone), dbuffer);
    for(uint32 curr_dentry = 0; curr_dentry < MINIX_BLOCK_SIZE; curr_dentry += DENTRY_SIZE)
    {
      if(dbuffer->get2Bytes(curr_dentry))
      {
        kprintfd("calling get Inode with the number: %d\n", dbuffer->get2Bytes(curr_dentry));
        Inode* inode = ((MinixFSSuperblock *)i_superblock_)->getInode( dbuffer->get2Bytes(curr_dentry) );
        kprintfd("returned get Inode with the number: %d\n", dbuffer->get2Bytes(curr_dentry));
        uint32 offset = 2;
        char *name = new char[MAX_NAME_LENGTH];
        char ch = 0;
        do
        {
          ch = dbuffer->getByte(curr_dentry + offset);
          name[offset] = ch;
          ++offset;
        } while (ch);
        // ? name[offset] = '\0';
        Dentry *new_dentry = new Dentry(name);
        // ? delete name
        i_dentry_->setChild(new_dentry);
        new_dentry->setParent(i_dentry_);
        (inode->getType() == I_DIR) ? inode->mkdir(new_dentry) : inode->mkfile(new_dentry);
      }
    }
  }
  delete dbuffer;
}


