/**
 * @file chardev.h
 */

#ifndef CHAR_DEV_H__
#define CHAR_DEV_H__

#include "fs/PointList.h"
#include "fs/Inode.h"
#include "fs/ramfs/RamFSFile.h"
#include "fs/devicefs/DeviceFSSuperblock.h"
#include "fs/Dentry.h"
#include "fs/Superblock.h"

#include "string.h"
#include "FiFo.h"

#include "Thread.h"

/**
 * @class CharacterDevice Links the character devices to the Device File System.
 */
class CharacterDevice : public Inode, public Thread
{
  public:

    /**
     * Constructor
     * @param name the device name
     * @param super_block the superblock (0)
     * @param inode_type the inode type (cahracter device)
     */
    CharacterDevice ( char* name, Superblock* super_block = 0, uint32 inode_type = I_CHARDEVICE ) :
        Inode ( super_block, inode_type )
    {

      i_type_   = I_CHARDEVICE;
      i_size_   = 0;
      i_nlink_  = 0;
      i_dentry_ = 0;

      uint32 name_len = strlen ( name ) + 1;
      device_name = new char[name_len];
      strlcpy ( device_name, name, name_len );

      i_superblock_ = DeviceFSSuperBlock::getInstance();
      DeviceFSSuperBlock::getInstance()->addDevice ( this, name );

      _in_buffer  = new FiFo< uint32 > ( CD_BUFFER_SIZE , FIFO_NOBLOCK_PUT | FIFO_NOBLOCK_PUT_OVERWRITE_OLD );
      _out_buffer = new FiFo< uint32 > ( CD_BUFFER_SIZE , FIFO_NOBLOCK_PUT | FIFO_NOBLOCK_PUT_OVERWRITE_OLD );
    };

    /**
     * Destructor
     */
    ~CharacterDevice()
    {
      if ( _in_buffer )
        delete _in_buffer;
      if ( _out_buffer )
        delete _out_buffer;

      name_ = "CharDevThread";
    };

    /**
     * reads the data from the character device
     * @param buffer is the buffer where the data is written to
     * @param count is the number of bytes to read.
     * @param offset is never to be used, because there is no offset
     *        in character devices, but it is defined in the Inode interface
     */
    virtual int32 readData ( int32 offset, int32 size, char *buffer )
    {
      if ( offset )
        return -1; // offset reading not supprted with char devices

      char *bptr = buffer;
      do
      {
        *bptr++ = _in_buffer->get();
      }
      while ( ( bptr - buffer ) < size );

      return ( bptr - buffer );
    };

    /**
     * writes to the character device
     * @param buffer is the buffer where the data is read from
     * @param count is the number of bytes to write.
     * @param offset is never to be used, because there is no offset
     *        in character devices, but it is defined in the Inode interface
     */
    virtual int32 writeData ( int32 offset, int32 size, const char*buffer )
    {
      if ( offset )
        return -1; // offset writing also not supp0rted

      const char *bptr = buffer;
      do
      {
        _out_buffer->put ( *bptr++ );
      }
      while ( ( bptr - buffer ) < size );

      return ( bptr - buffer );
    };

    /**
     * links the inode to the given dentry
     * @param dentry the denty
     * @return 0 on success
     */
    int32 mknod ( Dentry *dentry )
    {
      if ( dentry == 0 )
        return -1;

      i_dentry_ = dentry;
      dentry->setInode ( this );
      return 0;
    }

    /**
     * links the inode to the given dentry
     * @param dentry the denty
     * @return 0 on success
     */
    int32 create ( Dentry *dentry )
    {
      return ( mknod ( dentry ) );
    }

    /**
     * links the inode to the given dentry
     * @param dentry the denty
     * @return 0 on success
     */
    int32 mkfile ( Dentry *dentry )
    {
      return ( mknod ( dentry ) );
    }

    /**
     * Makes a hard link to the name referred to by the
     * denty, which is in the directory refered to by the Inode.
     * @param flag the flag
     * @return the linking File
     */
    File* link ( uint32 flag )
    {
      File* file = ( File* ) ( new RamFSFile ( this, i_dentry_, flag ) );
      i_files_.pushBack ( file );
      return file;
    }

    /**
     * removes the name refered to by the Dentry from the directory
     * referred to by the inode.
     * @param file the file to remove
     * @return 0 on success
     */
    int32 unlink ( File* file )
    {
      int32 tmp = i_files_.remove ( file );
      delete file;
      return tmp;
    }

    /**
     * processes the in and out buffers of the character device
     */
    virtual void Run()
    {
      do
      {
        processInBuffer();
        processOutBuffer();
      }
      while ( 1 );
    }

    FiFo< uint32 > *_in_buffer;
    FiFo< uint32 > *_out_buffer;

    char * device_name;

  private:

    /**
     * processes the in buffer of the character device
     */
    void processInBuffer()
    {
      if ( _in_buffer )
        _in_buffer->get();
    };


    /**
     * processes the in buffer of the character device
     */
    void processOutBuffer()
    {
      if ( _out_buffer )
        _out_buffer->get();
    };

    static const uint32 CD_BUFFER_SIZE = 1024;

};

#endif

