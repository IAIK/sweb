/**
 * Filename: RegularFile.cpp
 * Description:
 *
 * Created on: 21.07.2012
 * Author: chris
 */

#include "fs/inodes/RegularFile.h"

#include "fs/VfsSyscall.h"
#include "fs/FileSystem.h"
#include "fs/FsVolumeManager.h"
#include "fs/FileDescriptor.h"
#include "fs/FsDefinitions.h"

#ifdef USE_FILE_SYSTEM_ON_GUEST_OS
#include <cstring>
#endif

RegularFile::RegularFile(uint32 inode_number, uint32 device_sector,
    uint32 sector_offset, FileSystem* file_system, FsVolumeManager* volume_manager,
    unix_time_stamp access_time, unix_time_stamp mod_time, unix_time_stamp c_time,
    uint32 ref_count, uint32 size) : File(inode_number, device_sector,
    sector_offset, file_system, access_time, mod_time, c_time, ref_count, size),
    volume_manager_(volume_manager)
{
}

RegularFile::~RegularFile()
{
}

int32 RegularFile::read(FileDescriptor* fd, char* buffer, uint32 len)
{
  assert(fd != NULL);
  assert(fd->getFile() != NULL);

  // wrong file-instance!
  if(this != fd->getFile())
  {
    debug(FS_INODE, "RegularFile::read - wrong FileDescriptor asked for wrong file\n");
    return FileSystem::InvalidArgument;
  }

  // number of bytes successfully read (so far)
  uint32 read_bytes = 0;

  FileSystemLock* lock = fd->getFile()->getLock();

  // read() requires exclusive access, because of potential conflicts with
  // the file-cursor and the file-size
  if(fd->nonblockingMode())
  {
    debug(FS_INODE, "RegularFile::read - nonblocking mode\n");

    if(!lock->acquireReadNonBlocking())
    {
      debug(FS_INODE, "RegularFile::read - failed to assert mutual exclusion\n");
      return FileSystem::NonBlockAcquire;
    }
  }
  else
  {
    // wait until we obtain the lock
    lock->acquireReadBlocking();
  }

  // current cursor position
  file_size_t cursor_pos = fd->getCursorPos();

  while(read_bytes < len)
  {
    // determine on which block the next chunk of data is located
    uint32 sector_number = (cursor_pos + read_bytes) / file_system_->getDataBlockSize();
    sector_len_t sector_offset = (cursor_pos + read_bytes) % file_system_->getDataBlockSize();

    // getting the next sector
    sector_addr_t next_sector = getSector(sector_number);
    debug(FS_INODE, "RegularFile::read - reading sector=%X (sector is the %d th in the File)\n", next_sector, sector_number);

    // reading data-block
    volume_manager_->acquireDataBlockForReading(next_sector);

    char* block = volume_manager_->readDataBlockUnprotected(next_sector);
    if(block == NULL)
    {
      lock->releaseRead();
      if(read_bytes == 0) return FileSystem::IOReadError;
      else break;
    }

    // by default read everything from the offset to the end of the block ...
    sector_len_t num_bytes_to_cpy = file_system_->getDataBlockSize() - sector_offset;

    // ... but if that would be bigger than what the caller requested
    if(num_bytes_to_cpy > len - read_bytes)
    {
      num_bytes_to_cpy = len - read_bytes;
    }

    bool end_of_file = false;

    // avoid a size-overflow
    if(cursor_pos + read_bytes + num_bytes_to_cpy > getFileSize())
    {
      assert(getFileSize() > (cursor_pos + read_bytes));
      // fragment size is the rest of the file that should be read
      num_bytes_to_cpy = getFileSize() - (cursor_pos + read_bytes);
      end_of_file = true;
    }

    // deep-copy the sector's data into the callers buffer
    memcpy(buffer + read_bytes, block + sector_offset, num_bytes_to_cpy);

    volume_manager_->releaseReadDataBlock(next_sector);
    delete[] block;

    // update the number of read bytes
    read_bytes += num_bytes_to_cpy;

    if(end_of_file)
    {
      break;
    }
  }

  // update file cursor position
  fd->moveCursor(read_bytes);

  lock->releaseRead();

  // update time of last modification st_atime
  if(read_bytes > 0 && doUpdateAccessTime())
  {
    updateLastAccessTimeProtected(fd);
  }

  return read_bytes;
}

int32 RegularFile::lockWrite(FileDescriptor* fd)
{
  FileSystemLock* lock = fd->getFile()->getLock();

  // read() requires exclusive access, because of potential conflicts with
  // the file-cursor and the file-size
  if(fd->nonblockingMode())
  {
    debug(FS_INODE, "RegularFile::write - nonblocking mode\n");

    if(!lock->acquireWriteNonBlocking())
    {
      debug(FS_INODE, "RegularFile::write - failed to assert mutual exclusion\n");
      return FileSystem::NonBlockAcquire;
    }
  }
  else
  {
    // wait until we obtain the lock
    lock->acquireWriteBlocking();
  }

  return 0;
}

int32 RegularFile::write(FileDescriptor* fd, const char* buffer, uint32 len)
{
  assert(fd != NULL);
  assert(fd->getFile() != NULL);

  // wrong file-instance!
  if(this != fd->getFile())
  {
    debug(FS_INODE, "RegularFile::write - wrong FileDescriptor asked for wrong file\n");
    return FileSystem::InvalidArgument;
  }

  // number of bytes written
  uint32 written_bytes = 0;

  FileSystemLock* lock = fd->getFile()->getLock();

  // read() requires exclusive access, because of potential conflicts with
  // the file-cursor and the file-size
  if(fd->nonblockingMode())
  {
    debug(FS_INODE, "RegularFile::write - nonblocking mode\n");

    if(!lock->acquireWriteNonBlocking())
    {
      debug(FS_INODE, "RegularFile::write - failed to assert mutual exclusion\n");
      return FileSystem::NonBlockAcquire;
    }
  }
  else
  {
    // wait until we obtain the lock
    lock->acquireWriteBlocking();
  }

  // were there any changes to the file, so the I-Node on the Disk has
  // to be updated?
  bool inode_changed = false;

  // current cursor position
  file_size_t cursor_pos = fd->getCursorPos();

  if(fd->appendMode())
  {
    // file opened in append mode, set cursor to EOF before writing
    cursor_pos = getFileSize();
    debug(FS_INODE, "RegularFile::write - file in append mode, set cursor to EOF\n");
  }
  else if(len > 0 && cursor_pos > getFileSize())
  {
    debug(FS_INODE, "RegularFile::write - FileCursor > EOF\n");

    // file cursor offset is beyond the current-file size, so
    // the file needs to be resized
    file_size_t num_bytes_to_resize = cursor_pos - getFileSize();

    sector_addr_t num_blocks_to_request = (num_bytes_to_resize / file_system_->getDataBlockSize());

    if(num_bytes_to_resize % file_system_->getDataBlockSize() != 0)
      num_blocks_to_request++;

    debug(FS_INODE, "RegularFile::write - append %d empty blocks\n", num_blocks_to_request);

    for(sector_addr_t i = 0; i < num_blocks_to_request; i++)
    {
      // append missing sectors, and set all bytes to zero
      if(!file_system_->appendSectorToInode(this, true))
      {
        debug(FS_INODE, "RegularFile::write - FAIL FileSystem is full!\n");
        lock->releaseWrite();
        return FileSystem::FileSystemFull;
      }
      inode_changed = true;
    }

    // update size of File
    setFileSize(getFileSize() + num_bytes_to_resize);
  }

  while(written_bytes < len)
  {
    // determine the internal sector-number of the file that should be written to
    uint32 sector_number = (cursor_pos + written_bytes) / file_system_->getDataBlockSize();
    sector_len_t sector_offset = (cursor_pos + written_bytes) % file_system_->getDataBlockSize();

    debug(FS_INODE, "RegularFile::write - sector_nr=%x sector_offset=%d\n", sector_number, sector_offset);

    // getting the next sector
    sector_addr_t next_sector = getSector(sector_number);
    debug(FS_INODE, "RegularFile::write - next sector to write=%x\n", next_sector);

    if(next_sector == 0)
    {
      // file is not big enough, resize by requesting and adding a new block
      if(!file_system_->appendSectorToInode(this, true))
      {
        debug(FS_INODE, "RegularFile::write - FAIL FileSystem is full!\n");
        lock->releaseWrite();
        return FileSystem::FileSystemFull;
      }
      next_sector = getSector(sector_number);
      assert(next_sector != 0);
      inode_changed = true;
      debug(FS_INODE, "RegularFile::write - new sector (%x) successfully added!\n", next_sector);
    }

    // write next chunk of data to the sector

    // reading data-block
    volume_manager_->acquireDataBlockForWriting(next_sector);

    char* block = volume_manager_->readDataBlockUnprotected(next_sector);
    if(block == NULL)
    {
      lock->releaseWrite();
      return FileSystem::IOReadError;
    }

    sector_len_t bytes_to_write = file_system_->getDataBlockSize() - sector_offset;
    if(bytes_to_write > len - written_bytes)
    {
      // just write the rest and not until the end of the current sector
      bytes_to_write = len - written_bytes;
    }

    // update the block
    memcpy(block + sector_offset, buffer + written_bytes, bytes_to_write);

    // write updated sector back to the device
    if(!volume_manager_->writeDataBlockUnprotected(next_sector, block))
    {
      debug(FS_INODE, "RegularFile::write - FAILED to write updated sector!\n");
      lock->releaseWrite();
      delete[] block;

      return FileSystem::IOWriteError;
    }

    written_bytes += bytes_to_write;

    // release again
    volume_manager_->releaseWriteDataBlock(next_sector);

    // free allocated data-block
    delete[] block;
  }

  // update file-size and cursor position:
  if(cursor_pos + written_bytes > getFileSize())
    setFileSize( getFileSize() + (cursor_pos + written_bytes - getFileSize()) );

  fd->moveCursor(written_bytes);

  // update time of last change and c-time
  if(written_bytes > 0 || len > 0)
  {
    // file-size has had a change:
    inode_changed = true;

    unix_time_stamp t = VfsSyscall::getCurrentTimeStamp();
    updateModTime(t);
    updateCTime(t);
  }

  // there was a change performed to the I-Node, update it on the device
  if(inode_changed)
  {
    debug(FS_INODE, "RegularFile::write - file changed, rewrite i-node data to disk.\n");
    file_system_->writeInode(this);
  }

  lock->releaseWrite();
  return written_bytes;
}

bool RegularFile::truncateUnprotected(void)
{
  debug(FS_INODE, "RegularFile::truncateUnprotected - setting file-size to 0!\n");

  if(getFileSize() == 0)
  {
    debug(FS_INODE, "RegularFile::truncateUnprotected - file-size is already 0!\n");
    return false;
  }
/*
  // determine the number of sector the file has
  sector_addr_t calc_num_sectors = getFileSize() / file_system_->getDataBlockSize();
  if(getFileSize() % file_system_->getDataBlockSize() > 0) calc_num_sectors++;

  debug(FS_INODE, "RegularFile::truncateUnprotected - there should be %d sectors to free.\n", calc_num_sectors);

  // getting the number of currently used sectors
  sector_addr_t num_sectors = this->getNumSectors();

  debug(FS_INODE, "RegularFile::truncateUnprotected - there are exactly %d sectors to free.\n", num_sectors);

  if(num_sectors != calc_num_sectors)
  {
    debug(FS_INODE, "RegularFile::truncateUnprotected - ERROR calculated number of sectors does not match determined value!\n");
    return false;
  }
*/

  // free all sectors, belonging to the file
  for(sector_addr_t i = getNumSectors(); i > 0; i--)
  {
    if( !file_system_->removeLastSectorOfInode( this ) == 0 )
    {
      debug(FS_MINIX, "destroyInode - ERROR failed to free data-block!\n");
    }
  }

  // finally setting the file-size to 0:
  setFileSize(0);

  // queue a rewrite operation of the i-node
  file_system_->writeInode( this );

  debug(FS_INODE, "RegularFile::truncateUnprotected - DONE successfully truncated file.\n");
  return true;
}

bool RegularFile::updateLastAccessTimeProtected(FileDescriptor* fd)
{
  FileSystemLock* lock = fd->getFile()->getLock();

  if(fd->nonblockingMode())
  {
    if(!lock->acquireWriteNonBlocking())
    {
      debug(FS_INODE, "RegularFile::updateLastAccessTimeProtected - failed to establish mutual exclusion\n");
      return false;
    }
  }
  else
  {
    // wait until we obtain the lock
    lock->acquireWriteBlocking();
  }

  // update last access time of file
  updateAccessTime( VfsSyscall::getCurrentTimeStamp() );

  debug(FS_INODE, "RegularFile::updateLastAccessTimeProtected - updated access time of file.\n");

  lock->releaseWrite();
  return true;
}
