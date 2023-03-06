#include "fs/devicefs/DeviceFSSuperblock.h"
#include "fs/ramfs/RamFSInode.h"
#include "DeviceFSType.h"
#include "fs/Dentry.h"
#include "fs/Inode.h"
#include "fs/File.h"
#include "fs/FileDescriptor.h"
#include "Device.h"
#include "BDManager.h"
#include "BlockDeviceInode.h"
#include "BDVirtualDevice.h"

#include "debug.h"
#include "console/kprintf.h"

#include "Console.h"


class DeviceFSType;

extern Console* main_console;

const char DeviceFSSuperBlock::ROOT_NAME[] = "/";
const char DeviceFSSuperBlock::DEVICE_ROOT_NAME[] = "dev";

DeviceFSSuperBlock* DeviceFSSuperBlock::instance_ = nullptr;

DeviceFSSuperBlock::DeviceFSSuperBlock(DeviceFSType* fs_type, uint32 s_dev) :
    RamFSSuperblock(fs_type, s_dev)
{
}

DeviceFSSuperBlock::~DeviceFSSuperBlock()
{
}

void DeviceFSSuperBlock::addDevice(Inode* device_inode, const char* device_name)
{
  // Devices are mounted at the devicefs root (s_root_)
  device_inode->setSuperBlock(this);

  Dentry* fdntr = new Dentry(device_inode, s_root_, device_name);

  assert(device_inode->mknod(fdntr) == 0);
  all_inodes_.push_back(device_inode);
}

DeviceFSSuperBlock* DeviceFSSuperBlock::getInstance()
{
    if (!instance_)
        instance_ = new DeviceFSSuperBlock(DeviceFSType::getInstance(), 0);
    return instance_;
}

void DeviceFSSuperBlock::addBlockDeviceInodes()
{
    for (BDVirtualDevice* bdvd : BDManager::instance().device_list_)
    {
        debug(BD_VIRT_DEVICE, "Detected Device: %s :: %d\n", bdvd->getName(), bdvd->getDeviceNumber());
        kprintf("Detected Device: %s :: %d\n", bdvd->getName(), bdvd->getDeviceNumber());
        auto bdInode = new BlockDeviceInode(bdvd);
        addDevice(bdInode, bdvd->getName());
    }
}

void DeviceFSSuperBlock::addDeviceInodes(Device& device_root)
{
    const auto rec_lambda = [this](Device& device, auto& rec_func) -> void
    {
        if (auto device_inode = device.deviceInode())
        {
            debug(DRIVER, "Device %s has inode, adding to devicefs\n",
                device.deviceName().c_str());
            addDevice(device_inode, device.deviceName().c_str());
        }
        for (Device* sd : device.subdevices())
        {
            rec_func(*sd, rec_func);
        }
    };
    rec_lambda(device_root, rec_lambda);
}
