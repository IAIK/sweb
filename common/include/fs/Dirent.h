/**
 * @file Dirent.h
 */

#ifndef DIRENT_H__
#define DIRENT_H__

#include "fs/FsDefinitions.h"

#ifdef USE_FILE_SYSTEM_ON_GUEST_OS
#include <cstring>
#endif

/**
 * @class Dirent - general Dirent objects returned by the VfsSyscall
 * class for readdir() - method calls
 */
struct Dirent
{
  Dirent(inode_id_t inode_id, const char* filename) : inode_id_(inode_id), offset_(0), len_(0), type_(0)
  {
    if(filename != NULL)
    {
      size_t bytes_to_cpy = strlen(filename);
      if(bytes_to_cpy > NAME_MAX) bytes_to_cpy = NAME_MAX;

      strncpy(filename_, filename, 256);
      filename_[NAME_MAX] = '\0';
    }
    else
    {
      strncpy(filename_, "<untitled>", 10);
    }
  }

  inode_id_t inode_id_;
  uint32 offset_;
  uint16 len_;
  uint8 type_;
  char filename_[NAME_MAX + 1];
};

#endif // DIRENT_H__
