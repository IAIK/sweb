#include "BlockDeviceInode.h"
#include "BDVirtualDevice.h"
#include "File.h"
#include "Superblock.h"
#include "EASTL/memory.h"
#include "EASTL/unique_ptr.h"

BlockDeviceInode::BlockDeviceInode(BDVirtualDevice* device) :
    Inode(nullptr, I_BLOCKDEVICE),
    device_(device)
{
    debug(INODE, "New block device inode for device: %s\n", device->getName());
}

File* BlockDeviceInode::open(Dentry* dentry, uint32 flag)
{
    debug(INODE, "BlockDeviceInode: Open file\n");
    assert(eastl::find(i_dentrys_.begin(), i_dentrys_.end(), dentry) != i_dentrys_.end());

    File* file = (File*) (new SimpleFile(this, dentry, flag));
    i_files_.push_back(file);
    getSuperblock()->fileOpened(file);
    return file;
}

int32 BlockDeviceInode::readData(uint32 offset, uint32 size, char* buffer)
{
    size_t block_size = device_->getBlockSize();
    size_t bd_size = device_->getNumBlocks() * block_size;
    size_t read_size = eastl::min<size_t>(size, bd_size - offset);

    if ((offset % block_size == 0) && (read_size % block_size == 0))
    {
        return device_->readData(offset, read_size, buffer);
    }

    int32 bd_status = 0;

    auto tmp_buf = eastl::make_unique<char[]>(block_size);

    size_t num_read = 0;
    size_t read_loc = offset;
    while (num_read < read_size)
    {
        size_t num_remaining = read_size - num_read;
        size_t block_offset = read_loc % block_size;
        size_t chunk_size = eastl::min(num_remaining, block_size - block_offset);

        bd_status = device_->readData(read_loc - block_offset, block_size, tmp_buf.get());
        if (bd_status == -1)
        {
            break;
        }

        memcpy(buffer + num_read, tmp_buf.get() + block_offset, chunk_size);

        num_read += chunk_size;
        read_loc += chunk_size;
    }

    return num_read;
}

int32 BlockDeviceInode::writeData(uint32 offset, uint32 size, const char* buffer)
{
    size_t block_size = device_->getBlockSize();
    size_t bd_size = device_->getNumBlocks() * block_size;
    size_t write_size = eastl::min<size_t>(size, bd_size - offset);

    if ((offset % block_size == 0) && (write_size % block_size == 0))
    {
        return device_->writeData(offset, write_size, buffer);
    }

    int32 bd_status = 0;

    auto tmp_buf = eastl::make_unique<char[]>(block_size);

    size_t num_written = 0;
    size_t write_loc = offset;
    while (num_written < write_size)
    {
        size_t num_remaining = write_size - num_written;
        size_t block_offset = write_loc % block_size;
        size_t chunk_size = eastl::min(num_remaining, block_size - block_offset);

        if (block_offset != 0 || chunk_size != block_size)
        {
            bd_status = device_->readData(write_loc - block_offset, block_size, tmp_buf.get());
            if (bd_status == -1)
            {
                break;
            }
        }

        memcpy(tmp_buf.get() + block_offset, buffer + num_written, chunk_size);

        bd_status = device_->writeData(write_loc - block_offset, block_size, tmp_buf.get());
        if (bd_status == -1)
        {
            break;
        }

        num_written += chunk_size;
        write_loc += chunk_size;
    }

    return num_written;
}
