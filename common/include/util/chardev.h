#pragma once

#include "Device.h"
#include "FiFo.h"
#include "File.h"
#include "Inode.h"
#include "Superblock.h"
#include "EASTL/string.h"

class CharacterDevice : public Device, public Inode
{
  public:

    /**
     * Constructor
     * @param name the device name
     * @param super_block the superblock (0)
     * @param inode_type the inode type (character device)
     */
    CharacterDevice(const char* name) :
        Device(name),
        Inode(nullptr, I_CHARDEVICE),
        in_buffer_(CD_BUFFER_SIZE, FIFO_NOBLOCK_PUT | FIFO_NOBLOCK_PUT_OVERWRITE_OLD),
        out_buffer_(CD_BUFFER_SIZE, FIFO_NOBLOCK_PUT | FIFO_NOBLOCK_PUT_OVERWRITE_OLD),
        name_(name)
    {
    }

    ~CharacterDevice() override = default;

    Inode* deviceInode() override { return this; }

    File* open(Dentry* dentry, uint32 flag) override
    {
        debug(INODE, "CharacterDevice: Open file\n");
        assert(eastl::find(i_dentrys_.begin(), i_dentrys_.end(), dentry) !=
               i_dentrys_.end());

        File* file = (File*)(new NoOffsetFile(this, dentry, flag));
        i_files_.push_back(file);
        getSuperblock()->fileOpened(file);
        return file;
    }

    /**
     * reads the data from the character device
     * @param buffer is the buffer where the data is written to
     * @param count is the number of bytes to read.
     * @param offset is never to be used, because there is no offset
     *        in character devices, but it is defined in the Inode interface
     */
    int32 readData(uint32 offset, uint32 size, char *buffer) override
    {
      if (offset)
        return -1; // offset reading not supprted with char devices

      char *bptr = buffer;
      do
      {
        *bptr++ = in_buffer_.get();
      } while ((bptr - buffer) < (int32) size);

      return (bptr - buffer);
    }

    /**
     * writes to the character device
     * @param buffer is the buffer where the data is read from
     * @param count is the number of bytes to write.
     * @param offset is never to be used, because there is no offset
     *        in character devices, but it is defined in the Inode interface
     */
    int32 writeData(uint32 offset, uint32 size, const char*buffer) override
    {
      if (offset)
        return -1; // offset writing also not supp0rted

      const char *bptr = buffer;
      do
      {
        out_buffer_.put(*bptr++);
      } while ((bptr - buffer) < (int32) size);

      return (bptr - buffer);
    }

    [[nodiscard]] const char *getDeviceName() const
    {
      return name_.c_str();
    }

  protected:
    static const uint32 CD_BUFFER_SIZE = 1024;

    FiFo<uint8> in_buffer_;
    FiFo<uint8> out_buffer_;

    eastl::string name_;

    void processInBuffer()
    {
      in_buffer_.get();
    }

    void processOutBuffer()
    {
      out_buffer_.get();
    }

};
