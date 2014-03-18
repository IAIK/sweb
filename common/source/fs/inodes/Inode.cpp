/**
 * @file Inode.h
 */

#include "fs/inodes/Inode.h"
#include "fs/FileSystem.h"

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
#include "util/string.h"
//#include "MutexLock.h"
#else
#include <string>
#include <cstring>
#include <assert.h>
#endif

Inode::Inode(inode_id_t inode_number,
             sector_addr_t device_sector, sector_len_t sector_offset,
             FileSystem* file_system,
             unix_time_stamp access_time, unix_time_stamp mod_time,
             unix_time_stamp c_time,
             uint32 reference_conter, file_size_t size,
             uint32 uid, uint32 gid) :
  number_(inode_number), name_(NULL),
  device_sector_(device_sector), sector_offset_(sector_offset), data_sectors_(),
  //parent_(NULL),
  inode_lock_(NULL), file_system_(file_system),
  access_time_(access_time), mod_time_(mod_time), c_time_(c_time),
  uid_(uid), gid_(gid), permissions_(0755),
  reference_count_(reference_conter), size_(size)
{
  inode_lock_ = FileSystemLock::getNewFSLock();
}

Inode::Inode(const Inode& cpy) : number_(cpy.number_), name_(cpy.name_),
    device_sector_(cpy.device_sector_), sector_offset_(cpy.sector_offset_),
    data_sectors_(cpy.data_sectors_),
    //parent_(cpy.parent_),
    inode_lock_(NULL), file_system_(cpy.file_system_),
    access_time_(cpy.access_time_), mod_time_(cpy.mod_time_), c_time_(cpy.c_time_),
    uid_(cpy.uid_), gid_(cpy.gid_), permissions_(cpy.permissions_),
    reference_count_(cpy.reference_count_), size_(cpy.size_)
{
  inode_lock_ = FileSystemLock::getNewFSLock();
}

Inode::~Inode()
{
  debug(FS_INODE, "~Inode - CALL destroying Inode (%d).\n", getID());

  if(inode_lock_ != NULL)
  {
    delete inode_lock_;
    debug(FS_INODE, "~Inode - deleted inode-lock.\n");
  }

  if(name_ != NULL)
  {
    delete[] name_;
  }

  debug(FS_INODE, "~Inode - DONE\n");
}

FileSystemLock* Inode::getLock(void)
{
  return inode_lock_;
}

FileSystem* Inode::getFileSystem(void)
{
  return file_system_;
}

inode_id_t Inode::getID(void) const
{
  return number_;
}

void Inode::setName(const char* new_name)
{
  // if a name was already set, clear it first
  if(name_ != NULL)
  {
    delete[] name_;
    name_ = NULL;
  }

  if(new_name == NULL)
  {
    name_ = NULL;
  }
  else
  {
    name_ = strdup(new_name);
  }
}

const char* Inode::getName(void) const
{
  return name_;
}

unix_time_stamp Inode::getAccessTime(void) const
{
  return access_time_;
}

unix_time_stamp Inode::getModTime(void) const
{
  return mod_time_;
}

unix_time_stamp Inode::getCTime(void) const
{
  return c_time_;
}

void Inode::updateAccessTime(unix_time_stamp updated_access_t)
{
  access_time_ = updated_access_t;
}

void Inode::updateModTime(unix_time_stamp updated_mod_t)
{
  mod_time_ = updated_mod_t;
}

void Inode::updateCTime(unix_time_stamp updated_c_t)
{
  c_time_ = updated_c_t;
}

bool Inode::doUpdateAccessTime(void) const
{
  // generally no updates of a-times or just in special cases
  if( (file_system_->getMountFlags() & MS_NOATIME)
      || (getType() == Inode::InodeTypeDirectory && (file_system_->getMountFlags() & MS_NODIRATIME) ))
  {
    return false;
  }
  // always update the a-time or if MS_RELATIME is set and the conditions of
  // the flag are full-filled
  else if( (file_system_->getMountFlags() & MS_STRICTATIME) ||
      ((file_system_->getMountFlags() & MS_RELATIME)
          && (getAccessTime() < getModTime() || getAccessTime() < getCTime() )) )
  {
    return true;
  }

  return false;
}

void Inode::incrReferenceCount(void)
{
  if(reference_count_ < LINK_MAX)
    reference_count_++;
}

void Inode::decrReferenceCount(void)
{
  if(reference_count_ > 0)
    reference_count_--;
}

uint32 Inode::getReferenceCount(void) const
{
  return reference_count_;
}

file_size_t Inode::getFileSize(void) const
{
  return size_;
}

void Inode::setFileSize(file_size_t size)
{
  size_ = size;
}

uint32 Inode::getUID(void) const
{
  return uid_;
}

uint32 Inode::getGID(void) const
{
  return gid_;
}

void Inode::setUID(uint32 uid)
{
  uid_ = uid;
}

void Inode::setGID(uint32 gid)
{
  gid_ = gid;
}

void Inode::setPermissions(uint32 permissions)
{
  permissions_ = permissions;
}

uint32 Inode::getPermissions(void) const
{
  return permissions_;
}

sector_addr_t Inode::getSector(uint32 number)
{
  if(number >= data_sectors_.size())
  {
    // try to reload more sectors
    file_system_->updateInodesSectorList(this);

    // after reloading, again a sector with the given number is not
    // available ...
    if(number >= data_sectors_.size())
    {
      return 0; // non such a sector on the i-node
    }
  }

  return data_sectors_[number];
}

sector_addr_t Inode::getSector(uint32 number) const
{
  if(number >= data_sectors_.size())
  {
    return 0;
  }
  return data_sectors_[number];
}

void Inode::addSector(sector_addr_t sector)
{
  data_sectors_.push_back(sector);
}

void Inode::removeLastSector(void)
{
  data_sectors_.pop_back();
}

void Inode::removeSector(uint32 number)
{
  if(number >= data_sectors_.size())
    return;

  data_sectors_.erase( data_sectors_.begin() + number );
}

uint32 Inode::getNumSectors(void) const
{
  return data_sectors_.size();
}

void Inode::clearSectorList(void)
{
  data_sectors_.clear();
}

void Inode::setIndirectBlock(uint32 deg_of_indirection, sector_addr_t sector)
{
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  //indirect_blocks_.insert(ustl::make_pair(deg_of_indirection, sector));
  indirect_blocks_[deg_of_indirection] = sector; // TODO check ustl-insert implementation
#else
  indirect_blocks_[deg_of_indirection] = sector;
#endif
}

sector_addr_t Inode::getIndirectBlock(uint32 deg_of_indirection) const
{
  return indirect_blocks_.find( deg_of_indirection )->second;
/*#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  return indirect_blocks_[deg_of_indirection];
#else
  //std::map<uint32, sector_addr_t>::const_iterator it;
  //it = indirect_blocks_.find( deg_of_indirection );

  //return (*it)->second;
  return indirect_blocks_.find( deg_of_indirection )->second;
#endif*/
}

sector_addr_t Inode::getDeviceSector(void) const
{
  return device_sector_;
}

sector_len_t Inode::getSectorOffset(void) const
{
  return sector_offset_;
}
