#ifndef _CHAR_DEV_H_
#define _CHAR_DEV_H_

#include "fs/PointList.h"
#include "fs/Inode.h"
#include "fs/ramfs/RamFsFile.h"
#include "fs/devicefs/DeviceFSSuperblock.h"
#include "fs/Dentry.h"
#include "fs/Superblock.h"

#include "string.h"
#include "FiFo.h"

#include "Thread.h"

/// \class CharacterDevice
/// \brief Links the character devices to the Device File System.


class CharacterDevice : public Inode, public Thread
{
  public:
  CharacterDevice( char* name, Superblock* super_block = 0, uint32 inode_type = I_CHARDEVICE ) :
    Inode(super_block, inode_type) {
    
    i_type_   = I_CHARDEVICE;
    i_size_   = 0;
    i_nlink_  = 0;
    i_dentry_ = 0;

    uint32 name_len = strlen(name) + 1;
    device_name = new char[name_len];
    strlcpy(device_name, name, name_len);        
    
    i_superblock_ = DeviceFSSuperBlock::getInstance();
    DeviceFSSuperBlock::getInstance()->addDevice( this, name );
    
    _in_buffer  = new FiFo< uint32 >( CD_BUFFER_SIZE , FIFO_NOBLOCK_PUT | FIFO_NOBLOCK_PUT_OVERWRITE_OLD );  
    _out_buffer = new FiFo< uint32 >( CD_BUFFER_SIZE , FIFO_NOBLOCK_PUT | FIFO_NOBLOCK_PUT_OVERWRITE_OLD );
  };
  
  ~CharacterDevice() {
    if( _in_buffer )
      delete _in_buffer;
    if( _out_buffer )
      delete _out_buffer;
      
    name_ = "CharDevThread";
  };
   
  /// read the data from the character device
  ///
  /// @param buffer is the buffer where the data is written to
  /// @param count is the number of bytes to read.
  /// @param offset is never to be used, because there is no offset in character devices, but it is defined in the Inode interface
   virtual int32 readData(int32 offset, int32 size, char *buffer) {
    if( offset )
      return -1; // offset reading not supprted with char devices
      
    char *bptr = buffer;
    do {
      *bptr++ = _in_buffer->get();
    }
    while( (bptr - buffer) < size );
    
    return (bptr - buffer);
  };

  /// write to the character device
  ///
  /// @param buffer is the buffer where the data is read from
  /// @param count is the number of bytes to write.
  /// @param offset is never to be used, because there is no offset in character devices, but it is defined in the Inode interface
  virtual int32 writeData(int32 offset, int32 size, const char*buffer)  {
    if( offset )
      return -1; // offset writing also not supp0rted
    
    const char *bptr = buffer;
    do {
      _out_buffer->put( *bptr++ );
    }
    while( (bptr - buffer) < size );

    return (bptr - buffer);
  };
   
  int32 mknod(Dentry *dentry)
  {
    if(dentry == 0)
      return -1;
  
    i_dentry_ = dentry;
    dentry->setInode(this);
    return 0;
  } 
  
  int32 create(Dentry *dentry)
  {
    return(mknod(dentry));
  }
  
  int32 mkfile(Dentry *dentry)
  {
    return(mknod(dentry));
  }
  
  File* link(uint32 flag)
  {
    File* file = (File*)(new RamFsFile(this, i_dentry_, flag));
    i_files_.pushBack(file);
    return file;
  }

  int32 unlink(File* file)
  {
    int32 tmp = i_files_.remove(file);
    delete file;
    return tmp;
  }
  
  virtual void Run()
  {
    do 
    {
      processInBuffer();
      processOutBuffer();
    } while(1);
  }
 
  FiFo< uint32 > *_in_buffer;
  FiFo< uint32 > *_out_buffer;
  
  char * device_name;

  private:    
  
  void processInBuffer( void ) 
  { 
      if( _in_buffer) 
         _in_buffer->get(); 
  };
  
  void processOutBuffer( void )
  {
      if( _out_buffer) 
         _out_buffer->get(); 
  };

  static const uint32 CD_BUFFER_SIZE = 1024;
  
  protected:
};

#endif

