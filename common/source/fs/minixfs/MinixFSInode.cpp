
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
  UNUSED_ARG(i_uid);
  UNUSED_ARG(i_modtime);
  UNUSED_ARG(i_gid);
  i_size_ = i_size;
  i_nlink_ = i_nlinks;
  i_dentry_ = 0;
  i_state_ = I_UNUSED;
  i_zones_ = new MinixFSZone((MinixFSSuperblock *)super_block, i_zones);
  i_num_ = i_num;
  children_loaded_ = false;
  if(i_mode & 0x8000)
  {
    i_type_ = I_FILE;
  }
  else if(i_mode & 0x4000)
  {
    i_type_ = I_DIR;
  }
  //TODO: else something else (hard/sym link)

//   kprintfd( "created inode with size: %x\tnlink: %x\tzones[0]: %x\tmode: %x\n", i_size_, i_nlink_, i_zones_->getZone(0), i_mode);
}

//---------------------------------------------------------------------------
MinixFSInode::~MinixFSInode()
{
  delete i_zones_;
}

//---------------------------------------------------------------------------
int32 MinixFSInode::readData(uint32 offset, uint32 size, char *buffer)
{
  kprintfd("MinixFSInode readData> offset: %d, size; %d, buffer: %s\n",offset,size,buffer);
  if((size + offset) > i_size_)
  {
    kprintfd("the size is bigger than size of the file\n");
    assert(false);
  }
  uint32 zone = offset / ZONE_SIZE;
  uint32 zone_offset = offset % ZONE_SIZE;
  uint32 num_zones = (zone_offset + size) / ZONE_SIZE + 1;
  Buffer* rbuffer = new Buffer(ZONE_SIZE);
  
  uint32 index = 0;
  kprintfd("MinixFSInode readData> zone: %d, zone_offset %d, num_zones: %d\n",zone,zone_offset,num_zones);
  for(;zone < num_zones; zone++)
  {
    rbuffer->clear();
    kprintfd("MinixFSInode readData> calling readZone with the zone: %d\n", i_zones_->getZone(zone));
    ((MinixFSSuperblock *)i_superblock_)->readZone(i_zones_->getZone(zone), rbuffer);
    for(; index < size && (index + zone_offset) < ZONE_SIZE; index++)
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
  kprintfd("MinixFSInode writeData> offset: %d, size: %d, buffer: %s\n",offset,size,buffer);
  uint32 zone = offset / ZONE_SIZE;
  uint32 num_zones = (offset % ZONE_SIZE + size) / ZONE_SIZE + 1;
  uint32 last_used_zone = i_size_/ZONE_SIZE;
  uint32 last_zone = last_used_zone;
  kprintfd("MinixFSInode writeData> checking if to allocate new zones\n");
  if((size + offset) > i_size_)
  {
    kprintfd("MinixFSInode writeData> have to allocat new Zones === i_size_: %d\n",i_size_);
    uint32 num_new_zones = (size + offset - i_size_) / ZONE_SIZE + 1;
    kprintfd("MinixFSInode writeData> num_new_zones: %d\n",num_new_zones);
    for(uint32 new_zones = 0; new_zones < num_new_zones; new_zones++, last_zone++)
    {
      kprintfd("MinixFSInode writeData> new_zones: %d, last_zone: %d, num_new_zones: %d\n",new_zones,last_zone,num_new_zones);
      MinixFSSuperblock* sb = (MinixFSSuperblock*)i_superblock_;
      kprintfd("MinixFSInode writeData> allocating new Zone\n");
      uint16 new_zone = sb->allocateZone();
      kprintfd("MinixFSInode writeData> setting zone: %d, to %d\n",last_zone+1, new_zone);
      i_zones_->setZone(i_zones_->getNumZones(), new_zone);
    }
  }
  kprintfd("MinixFSInode writeData>check if to clean memory\n");
  if(offset > i_size_)
  {
    kprintfd("MinixFSInode writeData> have to clean memory\n");
    uint32 zone_size_offset =  i_size_%ZONE_SIZE;
    Buffer* fill_buffer = new Buffer(ZONE_SIZE);
    fill_buffer->clear();
    if (zone_size_offset)
    {
      readData( i_size_-zone_size_offset, zone_size_offset, fill_buffer->getBuffer());
      ((MinixFSSuperblock *)i_superblock_)->writeZone( last_used_zone, fill_buffer);
    }
    ++last_used_zone;
    for (; last_used_zone <= offset/ZONE_SIZE; last_used_zone++)
    {
      kprintfd("MinixFSInode writeData>cleaning memory\n");
      fill_buffer->clear();
      ((MinixFSSuperblock *)i_superblock_)->writeZone( last_used_zone, fill_buffer );
    }
    --last_used_zone;
    i_size_ = offset;
    delete fill_buffer;
  }
  uint32 zone_offset = offset%ZONE_SIZE;
  Buffer* wbuffer = new Buffer(num_zones * ZONE_SIZE);
  kprintfd("MinixFSInode writeData>reading data at the beginning of zone: offset-zone_offset: %d,zone_offset: %d\n",offset-zone_offset,zone_offset);
  readData( offset-zone_offset, zone_offset, wbuffer->getBuffer());
  for(uint32 index = 0, pos = zone_offset; index<size; pos++, index++)
  {
    kprintfd("MinixFSInode writeData>filling writebuffer\n");
    wbuffer->setByte( pos, buffer[index]);
  }
  kprintfd("MinixFSInode writeData> zone: %d, num_zones: %d\n",zone, num_zones);
  for(uint32 zone_index = 0; zone_index < num_zones; zone_index++)
  {
    kprintfd("MinixFSInode writeData>writing zone_index: %d, i_zones_->getZone(zone) : %d\n",zone_index,i_zones_->getZone(zone));
    wbuffer->setOffset(zone_index*ZONE_SIZE);
    ((MinixFSSuperblock *)i_superblock_)->writeZone(zone_index + i_zones_->getZone(zone), wbuffer);
  }
  if(i_size_ < offset + size)
  {
    i_size_ = offset + size;
  }
  kprintfd("MinixFSInode writeData>deleting writebuffer\n");
  delete wbuffer;
  kprintfd("MinixFSInode writeData>returning size: %d\n",size);
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

  ((MinixFSInode *)dentry->getParent()->getInode())->writeDentry(0, i_num_, i_dentry_->getName());
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
  i_dentry_ = dentry;
  dentry->setInode(this);
  
  ((MinixFSInode *)dentry->getParent()->getInode())->writeDentry(0, i_num_, i_dentry_->getName());

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
  i_dentry_ = dentry;
  kprintfd("MinixFSInode mkfile>dentry->getParent() : %d\n",dentry->getParent());
  kprintfd("MinixFSInode mkfile>dentry->getParent()->getInode() : %d\n",dentry->getParent()->getInode());
  kprintfd("MinixFSInode mkfile>i_dentry_->getName() : %s\n",i_dentry_->getName());
  ((MinixFSInode *)dentry->getParent()->getInode())->writeDentry(0, i_num_, i_dentry_->getName());
  i_dentry_->setInode(this);
  return 0;
}

int32 MinixFSInode::findDentry(uint32 i_num)
{
  kprintfd("MinixFSInode findDentry>\n");
  Buffer *dbuffer = new Buffer(ZONE_SIZE);
  for (uint32 zone = 0; zone < i_zones_->getNumZones(); zone++)
  {
    kprintfd("calling readZone with the zone: %d\n", i_zones_->getZone(zone));
    ((MinixFSSuperblock *)i_superblock_)->readZone(i_zones_->getZone(zone), dbuffer);
    for(uint32 curr_dentry = 0; curr_dentry < BLOCK_SIZE; curr_dentry += DENTRY_SIZE)
    {
      uint16 inode_index = dbuffer->get2Bytes(curr_dentry);
      if(inode_index == i_num)
      {
        return (zone * ZONE_SIZE + curr_dentry);
      }
    }
  }
  return -1;
}


void MinixFSInode::writeDentry(uint32 dest_i_num, uint32 src_i_num, char* name)
{
  kprintfd("MinixFSInode writeDentry> dest_i_num : %d, src_i_num : %d, name : %s\n", dest_i_num, src_i_num, name);
  assert(name);
  int32 dentry_pos = findDentry(dest_i_num);
  if(dentry_pos < 0 && dest_i_num == 0)
  {
    i_zones_->addZone(((MinixFSSuperblock *)i_superblock_)->allocateZone());
    dentry_pos = (i_zones_->getNumZones() - 1) * ZONE_SIZE;
  }
  Buffer *dbuffer = new Buffer(ZONE_SIZE);
  uint32 zone = i_zones_->getZone(dentry_pos / ZONE_SIZE);
  ((MinixFSSuperblock *)i_superblock_)->readZone(zone, dbuffer);
  kprintfd("MinixFSInode writeDentry> dentry_pos ZONE_SIZE : %d, src_i_num : %d\n", dentry_pos % ZONE_SIZE, src_i_num);
  dbuffer->set2Bytes(dentry_pos % ZONE_SIZE, src_i_num);
  kprintfd("MinixFSInode writeDentry> dbuffer->get2Bytes(dentry_pos  ZONE_SIZE) : %d\n",dbuffer->get2Bytes(dentry_pos % ZONE_SIZE));
  char ch = 'a'; // != '\0'
  for(uint32 offset = 0; offset < MAX_NAME_LENGTH;offset++)
  {
    if(ch != '\0')
    {
      ch = name[offset];
    }
    dbuffer->setByte(dentry_pos % ZONE_SIZE + offset + 2, ch);
  }
  dbuffer->print();
  ((MinixFSSuperblock *)i_superblock_)->writeZone(zone, dbuffer);
  delete dbuffer;
  return;
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

  if(dentry->emptyChild())
  {
    dentry->releaseInode();
    Dentry* parent_dentry = dentry->getParent();
    parent_dentry->childRemove(dentry);
    char ch = '\0';
    ((MinixFSInode *)parent_dentry->getInode())->writeDentry(((MinixFSInode *)dentry->getInode())->i_num_,0,&ch);
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

  if(dentry->emptyChild())
  {
    dentry->releaseInode();
    Dentry* parent_dentry = dentry->getParent();
    parent_dentry->childRemove(dentry);
    char ch = '\0';
    ((MinixFSInode *)parent_dentry->getInode())->writeDentry(((MinixFSInode *)dentry->getInode())->i_num_,0,&ch);
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
  if(children_loaded_)
    return;
  kprintfd("MinixFSInode loadChildren>\n");
  Buffer *dbuffer = new Buffer(ZONE_SIZE);
  for (uint32 zone = 0; zone < i_zones_->getNumZones(); zone++)
  {
    kprintfd("calling readZone with the zone: %d\n", i_zones_->getZone(zone));
    ((MinixFSSuperblock *)i_superblock_)->readZone(i_zones_->getZone(zone), dbuffer);
    for(uint32 curr_dentry = 0; curr_dentry < BLOCK_SIZE; curr_dentry += DENTRY_SIZE)
    {
      uint16 inode_index = dbuffer->get2Bytes(curr_dentry);
      if(inode_index)
      {
        kprintfd("calling get Inode with the number: %d\n", inode_index);
        Inode* inode = ((MinixFSSuperblock *)i_superblock_)->getInode( inode_index );
        kprintfd("returned get Inode with the number: %d\n", inode_index);
        uint32 offset = 0;
        char *name = new char[MAX_NAME_LENGTH];
        char ch = '\0';
        dbuffer->print();
        do
        {
          ch = dbuffer->getByte(curr_dentry + offset + 2);
          name[offset] = ch;
          ++offset;
          kprintfd("MinixFSInode loadChildren dentry ch: %c\n",ch);
        } while (ch);
        // ? name[offset] = '\0';
        kprintfd("MinixFSInode loadChildren dentry name: %s\n",name);
        Dentry *new_dentry = new Dentry(name);
        // ? delete name
        i_dentry_->setChild(new_dentry);
        new_dentry->setParent(i_dentry_);
        ((MinixFSInode *)inode)->i_dentry_ = new_dentry;
        new_dentry->setInode(inode);
      }
    }
  }
  children_loaded_ = true;
  delete dbuffer;
}

int32 MinixFSInode::flush()
{
  i_superblock_->writeInode(this);
  return 0;
}


