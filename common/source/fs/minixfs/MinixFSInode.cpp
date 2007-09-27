/**
 * @file MinixFSInode.cpp
 */

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


MinixFSInode::MinixFSInode(Superblock *super_block, uint32 inode_type) :
    Inode(super_block, inode_type)
{
  debug(M_INODE, "Simple Constructor\n");
  i_zones_ = 0;
  i_size_ = 0;
  i_nlink_ = 0;
  i_dentry_ = 0;
}


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

  debug(M_INODE, "Constructor: size: %d\tnlink: %d\tnum zones: %d\tmode: %x\n", i_size_, i_nlink_, i_zones_->getNumZones(), i_mode);
}


MinixFSInode::~MinixFSInode()
{
  debug(M_INODE, "Destructor\n");
  delete i_zones_;
}


int32 MinixFSInode::readData(uint32 offset, uint32 size, char *buffer)
{
  debug(M_INODE, "readData: offset: %d, size; %d,i_size_: %d\n",offset,size,i_size_);
  if((size + offset) > i_size_)
  {
    kprintfd("MinixFSInode: readData: the size is bigger than size of the file - aborting\n");
    assert(false);
  }
  uint32 zone = offset / ZONE_SIZE;
  uint32 zone_offset = offset % ZONE_SIZE;
  uint32 num_zones = (zone_offset + size) / ZONE_SIZE + 1;
  Buffer* rbuffer = new Buffer(ZONE_SIZE);

  uint32 index = 0;
  debug(M_INODE, "readData: zone: %d, zone_offset %d, num_zones: %d\n",zone,zone_offset,num_zones);
  for(;zone < num_zones; zone++)
  {
    rbuffer->clear();
    ((MinixFSSuperblock *)i_superblock_)->readZone(i_zones_->getZone(zone), rbuffer);
    for(uint32 read_index = 0; index < size && (read_index + zone_offset) < ZONE_SIZE; index++, read_index++)
    {
      buffer[index] = rbuffer->getByte( read_index + zone_offset);
    }
    zone_offset = 0;
  }
  delete rbuffer;
  return size;
}


int32 MinixFSInode::writeData(uint32 offset, uint32 size, const char *buffer)
{
  debug(M_INODE, "MinixFSInode writeData> offset: %d, size: %d, i_size_: %d\n",offset,size,i_size_);
  uint32 zone = offset / ZONE_SIZE;
  uint32 num_zones = (offset % ZONE_SIZE + size) / ZONE_SIZE + 1;
  uint32 last_used_zone = i_size_/ZONE_SIZE;
  uint32 last_zone = last_used_zone;
  if((size + offset) > i_size_)
  {
    uint32 num_new_zones = (size + offset - i_size_) / ZONE_SIZE + 1;
    for(uint32 new_zones = 0; new_zones < num_new_zones; new_zones++, last_zone++)
    {
      MinixFSSuperblock* sb = (MinixFSSuperblock*)i_superblock_;
      debug(M_INODE, "writeData: allocating new Zone\n");
      uint16 new_zone = sb->allocateZone();
      i_zones_->setZone(i_zones_->getNumZones(), new_zone);
    }
  }
  if(offset > i_size_)
  {
    debug(M_INODE, "writeData: have to clean memory\n");
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
      fill_buffer->clear();
      ((MinixFSSuperblock *)i_superblock_)->writeZone( last_used_zone, fill_buffer );
    }
    --last_used_zone;
    i_size_ = offset;
    delete fill_buffer;
  }
  uint32 zone_offset = offset%ZONE_SIZE;
  Buffer* wbuffer = new Buffer(num_zones * ZONE_SIZE);
  debug(M_INODE, "writeData: reading data at the beginning of zone: offset-zone_offset: %d,zone_offset: %d\n",offset-zone_offset,zone_offset);
  readData( offset-zone_offset, zone_offset, wbuffer->getBuffer());
  for(uint32 index = 0, pos = zone_offset; index<size; pos++, index++)
  {
    wbuffer->setByte( pos, buffer[index]);
  }
  for(uint32 zone_index = 0; zone_index < num_zones; zone_index++)
  {
    debug(M_INODE, "writeData: writing zone_index: %d, i_zones_->getZone(zone) : %d\n",zone_index,i_zones_->getZone(zone));
    wbuffer->setOffset(zone_index*ZONE_SIZE);
    ((MinixFSSuperblock *)i_superblock_)->writeZone(zone_index + i_zones_->getZone(zone), wbuffer);
  }
  if(i_size_ < offset + size)
  {
    i_size_ = offset + size;
  }
  delete wbuffer;
  return size;
}


int32 MinixFSInode::mknod(Dentry *dentry)
{
  debug(M_INODE, "mknod: dentry: %d, i_type_: %d\n",dentry,i_type_);
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


int32 MinixFSInode::mkdir(Dentry *dentry)
{
  debug(M_INODE, "mkdir: dentry: %d, i_type_: %d\n",dentry,i_type_);
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


int32 MinixFSInode::mkfile(Dentry *dentry)
{
  debug(M_INODE, "mkfile: dentry: %d, i_type_: %d\n",dentry,i_type_);
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
  ((MinixFSInode *)dentry->getParent()->getInode())->writeDentry(0, i_num_, i_dentry_->getName());
  i_dentry_->setInode(this);
  return 0;
}


int32 MinixFSInode::findDentry(uint32 i_num)
{
  debug(M_INODE, "findDentry: i_num: %d\n",i_num);
  Buffer *dbuffer = new Buffer(ZONE_SIZE);
  for (uint32 zone = 0; zone < i_zones_->getNumZones(); zone++)
  {
    ((MinixFSSuperblock *)i_superblock_)->readZone(i_zones_->getZone(zone), dbuffer);
    for(uint32 curr_dentry = 0; curr_dentry < BLOCK_SIZE; curr_dentry += DENTRY_SIZE)
    {
      uint16 inode_index = dbuffer->get2Bytes(curr_dentry);
      if(inode_index == i_num)
      {
        debug(M_INODE, "findDentry: found pos: %d\n",(zone * ZONE_SIZE + curr_dentry));
        return (zone * ZONE_SIZE + curr_dentry);
      }
    }
  }
  return -1;
}


void MinixFSInode::writeDentry(uint32 dest_i_num, uint32 src_i_num, char* name)
{
  debug(M_INODE, "writeDentry: dest_i_num : %d, src_i_num : %d, name : %s\n", dest_i_num, src_i_num, name);
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
  dbuffer->set2Bytes(dentry_pos % ZONE_SIZE, src_i_num);
  char ch = 'a'; // != '\0'
  for(uint32 offset = 0; offset < MAX_NAME_LENGTH;offset++)
  {
    if(ch != '\0')
    {
      ch = name[offset];
    }
    dbuffer->setByte(dentry_pos % ZONE_SIZE + offset + 2, ch);
  }
  ((MinixFSSuperblock *)i_superblock_)->writeZone(zone, dbuffer);
  delete dbuffer;
  return;
}


File* MinixFSInode::link(uint32 flag)
{
  debug(M_INODE, "link: flag: %d\n",flag);
  File* file = (File*)(new MinixFSFile(this, i_dentry_, flag));
  i_files_.pushBack(file);
  ++i_nlink_;
  return file;
}


int32 MinixFSInode::unlink(File* file)
{
  debug(M_INODE, "unlink\n");
  int32 tmp = i_files_.remove(file);
  delete file;
  --i_nlink_;
  return tmp;
}


int32 MinixFSInode::rmdir()
{
  debug(M_INODE, "rmdir\n");
  if(i_type_ != I_DIR)
    return -1;

  Dentry* dentry = i_dentry_;

  if(dentry->emptyChild())
  {
    Dentry* parent_dentry = dentry->getParent();
    parent_dentry->childRemove(dentry);
    char ch = '\0';
    ((MinixFSInode *)parent_dentry->getInode())->writeDentry(((MinixFSInode *)dentry->getInode())->i_num_,0,&ch);
    dentry->releaseInode();
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


int32 MinixFSInode::rm()
{
  if(i_files_.getLength() != 0)
  {
    debug(M_INODE, "the file is opened.\n");
    return -1;
  }

  Dentry* dentry = i_dentry_;
  if(dentry->emptyChild())
  {
    Dentry* parent_dentry = dentry->getParent();
    parent_dentry->childRemove(dentry);
    char ch = '\0';
    ((MinixFSInode *)parent_dentry->getInode())->writeDentry(((MinixFSInode *)dentry->getInode())->i_num_,0,&ch);;
    dentry->releaseInode();
    delete dentry;
    i_dentry_ = 0;
    debug(M_INODE, "rm: deleted\n");
    return INODE_DEAD;
  }
  else
  {
    // ERROR_DEC
    return -1;
  }
}


Dentry* MinixFSInode::lookup(const char* name)
{

  debug(M_INODE, "lookup: name: %s this->i_dentry_->getName(): %s \n",name,this->i_dentry_->getName());
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
      debug(M_INODE, "lookup: dentry_update->getName(): %s\n"),dentry_update->getName();
      if(((MinixFSInode *)dentry_update->getInode())->i_type_==I_DIR)
      {
        ((MinixFSInode *)dentry_update->getInode())->loadChildren();
      }
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
  {
    debug(M_INODE, "loadChildren: Children allready loaded\n");
    return;
  }
  Buffer *dbuffer = new Buffer(ZONE_SIZE);
  for (uint32 zone = 0; zone < i_zones_->getNumZones(); zone++)
  {
    ((MinixFSSuperblock *)i_superblock_)->readZone(i_zones_->getZone(zone), dbuffer);
    for(uint32 curr_dentry = 0; curr_dentry < BLOCK_SIZE; curr_dentry += DENTRY_SIZE)
    {
      uint16 inode_index = dbuffer->get2Bytes(curr_dentry);
      if(inode_index)
      {
        debug(M_INODE, "loadChildren: loading child %d\n", inode_index);
        Inode* inode = ((MinixFSSuperblock *)i_superblock_)->getInode( inode_index );
        ((MinixFSSuperblock *)i_superblock_)->all_inodes_.pushBack(inode);
        uint32 offset = 0;
        char *name = new char[MAX_NAME_LENGTH];
        char ch = '\0';
//         dbuffer->print();
        do
        {
          ch = dbuffer->getByte(curr_dentry + offset + 2);
          name[offset] = ch;
          ++offset;
        } while (ch);
        debug(M_INODE, "loadChildren: dentry name: %s\n",name);
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
  debug(M_INODE, "flush: flushed\n");
  return 0;
}


