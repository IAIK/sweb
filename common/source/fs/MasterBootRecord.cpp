#include "MasterBootRecord.h"
#include "BDDriver.h"
#include "BDManager.h"
#include "BDVirtualDevice.h"
#include "ArchInterrupts.h"
#include "debug.h"
#include "EASTL/array.h"
#include "EASTL/string.h"

int detectMBRPartitions(BDVirtualDevice* bdv, BDDriver* drv, uint32_t sector, uint32_t SPT, const char* name)
{
    uint32 offset = 0, numsec = 0;
    eastl::array<char, 512> buff; // read buffer
    debug(IDE_DRIVER, "processMBR:reading MBR\n");

    static uint32 part_num = 0;

    int32 read_res = bdv->readData(sector*drv->getSectorSize(), 512, buff.data());

    if (read_res != 512)
    {
        debug(IDE_DRIVER, "processMBR: drv returned BD_ERROR\n");
        return -1;
    }

    MasterBootRecord* mbr = (MasterBootRecord*)buff.data();

    if (mbr->signature == MasterBootRecord::PC_MBR_SIGNATURE)
    {
        debug(IDE_DRIVER, "processMBR: | Valid PC MBR | \n");
        for (int i = 0; MasterBootRecord::PartitionEntry& fp : mbr->parts)
        {
            debug(IDE_DRIVER,
                  "partition %u: type %x [%s] at sectors [%d -> %d), num sectors: %d, "
                  "bytesize: %u, bootable: %d\n",
                  i, fp.type, partitionTypeName(fp.type), fp.first_sector_lba,
                  fp.first_sector_lba + fp.num_sectors, fp.num_sectors,
                  fp.num_sectors * drv->getSectorSize(),
                  fp.bootable ==
                      MasterBootRecord::PartitionEntry::BootableStatus::BOOTABLE);

            switch (fp.type)
            {
            case EMPTY:
                // do nothing
                break;
            case DOS_EXTENDED_CHS:
            case WINDOWS_EXTENDED_LBA:
            case LINUX_EXTENDED:
            {
                if (detectMBRPartitions(bdv, drv, sector + fp.first_sector_lba, SPT, name) == -1)
                    detectMBRPartitions(bdv, drv, sector + fp.num_sectors - SPT, SPT, name);
                break;
            }
            case MINIXFS_ALT:
            case MINIXFS_OLD:
            case MINIXFS:
            case SWAP:
            case LINUX_ANY_NATIVE:
            default:
            {
                // offset = fp->relsect - SPT;
                offset = fp.first_sector_lba;
                numsec = fp.num_sectors;

                eastl::string part_name{name};
                part_name += eastl::to_string(part_num);
                part_num++;
                BDVirtualDevice* bdv = new BDVirtualDevice(
                    drv, offset, numsec, drv->getSectorSize(), part_name.c_str(), true);

                // set Partition Type (FileSystem identifier)
                bdv->setPartitionType(fp.type);

                BDManager::instance().addVirtualDevice(bdv);
                break;
            }
            }

            ++i;
        }
    }
    else
    {
        debug(IDE_DRIVER, "processMBR: | Invalid PC MBR %d | \n", mbr->signature);
        return -1;
    }

    debug(IDE_DRIVER, "processMBR:, done with partitions \n");
    return 0;
}
