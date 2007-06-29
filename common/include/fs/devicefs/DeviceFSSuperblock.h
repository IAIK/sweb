#ifndef _DEVICEFS_SUPERBLOCK_H_
#define _DEVICEFS_SUPERBLOCK_H_

#include "fs/PointList.h"
#include "fs/Superblock.h"

class Inode;
class Superblock;
class CharacterDevice;

class DeviceFSSuperBlock : public Superblock
{
  public:
  static const char ROOT_NAME[];
  static const char DEVICE_ROOT_NAME[];

  DeviceFSSuperBlock(Dentry* s_root, uint32 s_dev);
  virtual ~DeviceFSSuperBlock();

  /// create a new Inode of the superblock
  virtual Inode* createInode(Dentry*, uint32 );
  virtual int32  createFd(Inode*, uint32 );
  
  void addDevice( Inode*, char* );  
  // singleton
  static DeviceFSSuperBlock* getInstance() 
  {
    if( !instance_ )
      instance_ = new DeviceFSSuperBlock( 0 , 0 );
    return instance_;
  }

  private:
  Inode*   cDevice;
  Dentry*  s_dev_dentry_;
  
  protected:
  static DeviceFSSuperBlock* instance_;
  
};

#endif
