/**
 * Filename: DIR.h
 * Description:
 *
 * Created on: 15.08.2012
 * Author: chris
 */

#ifndef DIR_H_
#define DIR_H_

#include "fs/FileSystem.h"
#include "fs/inodes/Directory.h"

/**
 * DIR-structure
 */
struct DIR
{
  DIR(Directory* dir) : dir_(dir), cursor_pos(0)
  {
  }

  virtual ~DIR()
  {
    if(dir_ != NULL)
    {
      FileSystem* fs = dir_->getFileSystem();
      fs->releaseInode(dir_);
    }
  }

  Directory* dir_;      // the opened-directory
  uint32 cursor_pos;    // the current position of the cursor
};


#endif /* DIR_H_ */
