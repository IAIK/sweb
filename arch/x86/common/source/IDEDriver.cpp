#include "IDEDriver.h"

#include "ATADriver.h"
#include "BDManager.h"
#include "BDVirtualDevice.h"
#include "MasterBootRecord.h"
#include "kprintf.h"
#include "kstring.h"
#include "ports.h"
#include "ArchInterrupts.h"

IDEDriver::IDEDriver() :
    DeviceDriver("IDE Driver")
{
    doDeviceDetection();
}

IDEDriver& IDEDriver::instance()
{
    static IDEDriver instance_;
    return instance_;
}

void IDEDriver::doDeviceDetection()
{
    uint32 jiffies = 0;
    uint16 base_port = 0x1F0;
    uint16 base_regport = 0x3F6;
    uint8 cs = 0;
    uint8 sc;
    uint8 sn;
    uint8 devCtrl;

    uint8 ata_irqs[4] = {14, 14, 15, 15};

    // setup register values
    devCtrl = 0x00; // please use interrupts

    // assume there are no devices
    debug(IDE_DRIVER, "doDetection:%d\n", cs);

    for (cs = 0; cs < 4; cs++)
    {
        eastl::string name{"ide"};
        name += 'a' + cs;

        debug(IDE_DRIVER, "doDetection:Detecting IDE DEV: %s\n", name.c_str());

        if (cs > 1)
        {
            base_port = 0x170;
            base_regport = 0x376;
        }

        outportbp(base_regport, devCtrl); // init the device with interupts

        uint8 value = (cs % 2 == 0 ? 0xA0 : 0xB0);
        uint16 bpp6 = base_port + 6;
        uint16 bpp2 = base_port + 2;
        uint16 bpp3 = base_port + 3;

        outportb(bpp6, value);
        outportb(0x80, 0x00);

        outportb(bpp2, 0x55);
        outportb(0x80, 0x00);
        outportb(bpp3, 0xAA);
        outportb(0x80, 0x00);
        outportb(bpp2, 0xAA);
        outportb(0x80, 0x00);
        outportb(bpp3, 0x55);
        outportb(0x80, 0x00);
        outportb(bpp2, 0x55);
        outportb(0x80, 0x00);
        outportb(bpp3, 0xAA);
        outportb(0x80, 0x00);

        sc = inportb(bpp2);
        outportb(0x80, 0x00);
        sn = inportb(bpp3);
        outportb(0x80, 0x00);

        if ((sc == 0x55) && (sn == 0xAA))
        {
            outportbp(base_regport, devCtrl | 0x04); // RESET
            outportbp(base_regport, devCtrl);

            jiffies = 0;
            while (!(inportbp(base_port + 7) & 0x58) && jiffies++ < IO_TIMEOUT)
                ArchInterrupts::yieldIfIFSet();

            if (jiffies >= IO_TIMEOUT)
                debug(IDE_DRIVER, "doDetection: Still busy after reset!\n ");
            else
            {
                outportbp(base_port + 6, (cs % 2 == 0 ? 0xA0 : 0xB0));

                uint8 c1 = inportbp(base_port + 2);
                uint8 c2 = inportbp(base_port + 3);

                if (c1 != 0x01 && c2 != 0x01)
                    debug(IDE_DRIVER, "doDetection: Not found after reset ! \n");
                else
                {
                    uint8 c3 = inportbp(base_port + 7);
                    uint8 c4 = inportbp(base_port + 4);
                    uint8 c5 = inportbp(base_port + 5);

                    if (((c4 == 0x14) && (c5 == 0xEB)) || ((c4 == 0x69) && (c5 == 0x96)))
                    {
                        debug(IDE_DRIVER, "doDetection: Found ATAPI ! \n");
                        debug(IDE_DRIVER, "doDetection: port: %4X, drive: %d \n",
                              base_port, cs % 2);

                        debug(IDE_DRIVER, "doDetection: CDROM not supported \n");

                        // CDROM hook goes here
                        //
                        // char *name = "ATAX0";
                        // name[3] = cs + '0';
                        // drv = new CROMDriver ( base_port, cs % 2 );
                        // BDVirtualDevice *bdv = new
                        // BDVirtualDevice( drv, 0, drv->getNumSectors(),
                        // drv->getSectorSize(), name, true);
                        // BDManager::getInstance()->addDevice( bdv );
                    }
                    else
                    {
                        if (c3 != 0)
                        {
                            if ((c4 == 0x00) && (c5 == 0x00))
                            {
                                debug(IDE_DRIVER, "doDetection: Found PATA ! \n");
                                debug(IDE_DRIVER, "doDetection: port: %4X, drive: %d \n",
                                      base_port, cs % 2);

                                ATADriver* drv =
                                    new ATADriver(base_port, cs % 2, ata_irqs[cs]);
                                BDVirtualDevice* bdv =
                                    new BDVirtualDevice(drv, 0, drv->getNumSectors(),
                                                        drv->getSectorSize(), name.c_str(), true);

                                BDManager::instance().addVirtualDevice(bdv);
                                processMBR(drv, 0, drv->SPT, name.c_str());
                            }
                            else if ((c4 == 0x3C) && (c5 == 0xC3))
                            {
                                debug(IDE_DRIVER, "doDetection: Found SATA device! \n");
                                debug(IDE_DRIVER, "doDetection: port: %4X, drive: %d \n",
                                      base_port, cs % 2);

                                // SATA hook
                                // drv = new SATADriver ( base_port, cs % 2 );

                                debug(IDE_DRIVER, "doDetection: Running SATA device as "
                                                  "PATA in compatibility mode! \n");

                                ATADriver* drv =
                                    new ATADriver(base_port, cs % 2, ata_irqs[cs]);

                                BDVirtualDevice* bdv =
                                    new BDVirtualDevice(drv, 0, drv->getNumSectors(),
                                                        drv->getSectorSize(), name.c_str(), true);

                                BDManager::instance().addVirtualDevice(bdv);

                                processMBR(drv, 0, drv->SPT, name.c_str());
                            }
                        }
                        else
                        {
                            debug(IDE_DRIVER, "doDetection: Unknown harddisk!\n");
                        }
                    }
                }
            }
        }
        else
        {
            debug(IDE_DRIVER, "doDetection: Not found!\n ");
        }
    }
}

int32 IDEDriver::processMBR(BDDriver* drv, uint32 sector, uint32 SPT, const char* name)
{
    uint32 offset = 0, numsec = 0;
    uint16 buff[256]; // read buffer
    debug(IDE_DRIVER, "processMBR:reading MBR\n");

    static uint32 part_num = 0;

    uint32 read_res = ((ATADriver*)drv)->rawReadSector(sector, 1, (void*)buff);

    if (read_res != 0)
    {
        debug(IDE_DRIVER, "processMBR: drv returned BD_ERROR\n");
        return -1;
    }

    MasterBootRecord* mbr = (MasterBootRecord*)buff;

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
                if (processMBR(drv, sector + fp.first_sector_lba, SPT, name) == -1)
                    processMBR(drv, sector + fp.num_sectors - SPT, SPT, name);
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
