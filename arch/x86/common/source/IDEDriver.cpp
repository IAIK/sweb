#include "IDEDriver.h"

#include "ATACommands.h"
#include "ATADriver.h"
#include "BDManager.h"
#include "BDVirtualDevice.h"
#include "MasterBootRecord.h"
#include "kprintf.h"
#include "kstring.h"
#include "ports.h"
#include "source_location.h"
#include "ArchInterrupts.h"

size_t IDEControllerChannel::num_ide_controllers = 0;

IDEControllerDriver::IDEControllerDriver() :
    BasicDeviceDriver("IDE Driver")
{
}

IDEControllerDriver& IDEControllerDriver::instance()
{
    static IDEControllerDriver instance_;
    return instance_;
}

void IDEControllerDriver::doDeviceDetection()
{
    // Assume we have an IDE controller
    // Normally detcted via PCI bus enumeration
    auto ide_controller = new IDEController("IDE Controller");
    bindDevice(*ide_controller);
}

int32 IDEControllerDriver::detectPartitions(BDDriver* drv, uint32 sector, uint32 SPT, const char* name)
{
    uint32 offset = 0, numsec = 0;
    eastl::array<uint16_t, 256> buff; // read buffer
    debug(IDE_DRIVER, "processMBR:reading MBR\n");

    static uint32 part_num = 0;

    uint32 read_res = ((ATADrive*)drv)->rawReadSector(sector, 1, buff.data());

    if (read_res != 0)
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
                if (detectPartitions(drv, sector + fp.first_sector_lba, SPT, name) == -1)
                    detectPartitions(drv, sector + fp.num_sectors - SPT, SPT, name);
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

IDEControllerChannel::IDEControllerChannel(const eastl::string& name,
                                           uint16_t io_reg_base,
                                           uint16_t control_reg_base,
                                           uint16_t isa_irqnum) :
    DeviceBus(name),
    IrqDomain(name),
    io_regs({io_reg_base}),
    control_regs({control_reg_base}),
    isa_irqnum(isa_irqnum),
    controller_id(num_ide_controllers++)
{
    debug(IDE_DRIVER, "Init IDE Controller Channel %s\n", deviceName().c_str());

    registerDriver(PATADeviceDriver::instance());

    IrqDomain::irq()
        .mapTo(ArchInterrupts::isaIrqDomain(), isa_irqnum);

    doDeviceDetection();
}

void IDEControllerChannel::reset()
{
    debug(IDE_DRIVER, "Reset controller\n");
    ControlRegister::DeviceControl dc{};
    dc.reset = 1;
    control_regs[ControlRegister::DEVICE_CONTROL].write(dc);
    dc.reset = 0;
    control_regs[ControlRegister::DEVICE_CONTROL].write(dc);

    waitNotBusy();

    // Drive 0 is normally automatically selected after reset, but this might not automatically be done by QEMU
    selectDrive(0, true);
}

bool IDEControllerChannel::waitDriveReady(source_location loc)
{
    int jiffies = 0;
    auto status = control_regs[IDEControllerChannel::ControlRegister::ALT_STATUS].read();
    debugAdvanced(IDE_DRIVER, "Wait for drive to be ready. Status: %x\n",
                  status.u8);
    while ((status.busy || !status.ready) && jiffies++ < IO_TIMEOUT)
    {
        status = control_regs[IDEControllerChannel::ControlRegister::ALT_STATUS].read();
        ++jiffies;
    }

    if (status.error)
    {
        auto error = io_regs[IDEControllerChannel::IoRegister::ERROR].read();

        debugAlways(IDE_DRIVER,
                    "Drive reported error while waiting for drive to be ready. "
                    "Status: %x, error: %x. "
                    "At %s:%u %s\n"
                    "\n",
                    status.u8, error.u8, loc.file_name(), loc.line(),
                    loc.function_name());

        return false;
    }

    if (jiffies >= IO_TIMEOUT)
    {
        debugAlways(IDE_DRIVER,
                    "ERROR: Timeout while waiting for IDE drive to be ready. "
                    "Status: %x. "
                    "At %s:%u %s\n",
                    status.u8, loc.file_name(), loc.line(), loc.function_name());
    }

    return status.ready;
}

bool IDEControllerChannel::waitNotBusy(source_location loc)
{
    int jiffies = 0;
    auto status = control_regs[IDEControllerChannel::ControlRegister::ALT_STATUS].read();
    debugAdvanced(IDE_DRIVER, "Wait until drive is not busy. Status: %x\n", status.u8);
    while (status.busy && jiffies++ < IO_TIMEOUT)
    {
        status = control_regs[IDEControllerChannel::ControlRegister::ALT_STATUS].read();
        ++jiffies;
    }

    if (status.error)
    {
        auto error = io_regs[IDEControllerChannel::IoRegister::ERROR].read();

        debugAlways(IDE_DRIVER,
                    "Drive reported error while waiting until drive is not busy. "
                    "Status: %x, error: %x.\n"
                    "At %s:%u %s\n",
                    status.u8, error.u8, loc.file_name(), loc.line(),
                    loc.function_name());

        return false;
    }

    if (jiffies >= IO_TIMEOUT)
    {
        debugAlways(IDE_DRIVER,
                    "ERROR: Timeout while waiting until drive is not busy. "
                    "Status: %x.\n"
                    "At %s:%u %s\n",
                    status.u8, loc.file_name(), loc.line(), loc.function_name());
    }

    return !status.busy;
}

bool IDEControllerChannel::waitDataReady(source_location loc)
{
    int jiffies = 0;
    auto status = control_regs[IDEControllerChannel::ControlRegister::ALT_STATUS].read();
    debugAdvanced(IDE_DRIVER, "Wait for data ready. Status: %x\n", status.u8);
    while (status.busy && jiffies++ < IO_TIMEOUT)
    {
        status = control_regs[IDEControllerChannel::ControlRegister::ALT_STATUS].read();
        ++jiffies;
    }

    if (status.error)
    {
        auto error = io_regs[IDEControllerChannel::IoRegister::ERROR].read();

        debugAlways(IDE_DRIVER,
                    "Drive reported error while waiting for data to be available. "
                    "Status: %x, error: %x\n",
                    status.u8, error.u8);

        return false;
    }
    if (!status.data_ready)
    {
        auto error = io_regs[IDEControllerChannel::IoRegister::ERROR].read();

        debugAlways(IDE_DRIVER, "Data not ready. Status: %x, error: %x\n", status.u8,
                    error.u8);
    }

    if (jiffies >= IO_TIMEOUT)
    {
        debugAlways(IDE_DRIVER,
                    "ERROR: Timeout while waiting for IDE controller data ready.\n"
                    "At %s:%u %s\n",
                    loc.file_name(), loc.line(), loc.function_name());
    }

    return status.data_ready;
}


void IDEControllerChannel::sendCommand(uint8_t cmd)
{
    debugAdvanced(IDE_DRIVER, "Send command: %x\n", cmd);
    io_regs[IoRegister::COMMAND].write(cmd);
}

uint8_t IDEControllerChannel::selectedDrive()
{
    return selected_drive;
}

void IDEControllerChannel::selectDrive(uint8_t drive, bool force)
{
    assert(drive <= 1);

    if (force || selected_drive != drive)
    {
        IoRegister::DriveHead dh{};
        dh.drive_num = drive;
        dh.use_lba = 1;
        dh.always_set_0 = 1;
        dh.always_set_1 = 1;
        debugAdvanced(ATA_DRIVER, "Select %s drive %u\n", deviceName().c_str(), drive);
        io_regs[IoRegister::DRIVE_HEAD].write(dh);

        // Delay so drive select is registered before next command
        for (size_t i = 0; i < 15; ++i)
            control_regs[ControlRegister::ALT_STATUS].read();
    }

    selected_drive = drive;
}

bool IDEControllerChannel::detectChannel(uint16_t io_reg_base,
                                         uint16_t control_reg_base)
{
    IoPort::IoRegisterSet io_regs{io_reg_base};

    debug(IDE_DRIVER, "IDE Channel %x:%x detection\n", io_reg_base, control_reg_base);
    auto status = io_regs[IoRegister::STATUS].read();
    if (status.u8 == 0xFF)
    {
        debug(IDE_DRIVER, "Floating bus detected, channel empty\n");
        return false;
    }

    // Detect presence of I/O ports by writing to them and trying to read back
    // the written value
    constexpr uint8_t test_sc = 0x55;
    constexpr uint8_t test_sn = 0xAA;
    io_regs[IoRegister::SECTOR_COUNT].write(test_sc);
    io_regs[IoRegister::SECTOR_NUMBER].write(test_sn);

    auto sc = io_regs[IoRegister::SECTOR_COUNT].read();
    auto sn = io_regs[IoRegister::SECTOR_NUMBER].read();
    if (sc == test_sc && sn == test_sn)
    {
        debug(IDE_DRIVER, "Channel %x:%x exists\n", io_reg_base, control_reg_base);
        return true;
    }
    else
    {
        debug(IDE_DRIVER, "Channel %x:%x does not exists\n", io_reg_base,
              control_reg_base);
        return false;
    }
}


void IDEControllerChannel::doDeviceDetection()
{
    debug(IDE_DRIVER, "IDE Channel device detection\n");

    for (uint8_t disk = 0; disk < 2; ++disk)
    {
        detectDrive(disk);
    }
}

void IDEControllerChannel::detectDrive(uint8_t drive_num)
{
    debug(ATA_DRIVER, "Reset %s drives\n", deviceName().c_str());
    reset();

    // Select drive again after reset
    selectDrive(drive_num);

    // Send IDENTIFY command
    debug(ATA_DRIVER, "Send IDENTIFY command\n");
    io_regs[IoRegister::COMMAND].write(ATACommand::PIO::IDENTIFY_DEVICE);

    auto status = control_regs[IDEControllerChannel::ControlRegister::ALT_STATUS].read();
    if (status.u8 == 0)
    {
        debug(ATA_DRIVER, "IDENTIFY: Drive %u does not exist\n", drive_num);
    }
    else
    {
        debug(ATA_DRIVER, "IDENTIFY: Drive %u exists\n", drive_num);
        waitNotBusy();
        status = control_regs[IDEControllerChannel::ControlRegister::ALT_STATUS].read();

        auto lba_mid = io_regs[IoRegister::LBA_MID].read();
        auto lba_high = io_regs[IoRegister::LBA_HIGH].read();
        if (!status.error && (lba_mid != 0 || lba_high != 0))
        {
            debug(ATA_DRIVER,
                  "IDENTIFY: Non spec-conforming ATAPI device found, aborting\n");
            return;
        }

        waitDataReady();
        status = control_regs[IDEControllerChannel::ControlRegister::ALT_STATUS].read();

        /*
          See ATA command set https://people.freebsd.org/~imp/asiabsdcon2015/works/d2161r5-ATAATAPI_Command_Set_-_3.pdf table 206 for device signatures
         */
        auto count = io_regs[IoRegister::SECTOR_COUNT].read();
        auto lba_low = io_regs[IoRegister::LBA_LOW].read();
        lba_mid = io_regs[IoRegister::LBA_MID].read();
        lba_high = io_regs[IoRegister::LBA_HIGH].read();

        debug(ATA_DRIVER, "IDENTIFY command signature: status: %x, count: %x, lba h: %x, lba m: %x, lba l: %x\n", status.u8, count, lba_high, lba_mid, lba_low);

        bool found_driver = probeDrivers(IDEDeviceDescription{this, drive_num, {count, lba_low, lba_mid, lba_high}});
        if (found_driver)
        {
            debug(ATA_DRIVER, "Found driver for device: %u\n", found_driver);
        }
        else
        {
            debug(ATA_DRIVER, "Could not find driver for device\n");
        }

        if (status.error)
        {
            debug(ATA_DRIVER, "IDENTIFY command aborted: Not an ATA device\n");

            if (count == 0x01 && lba_low == 0x01 && lba_mid == 0x14 && lba_high == 0xEB)
            {
                debug(ATA_DRIVER, "IDENTIFY command aborted: ATAPI device found\n");
            }
            else if (count == 0x01 && lba_low == 0x01 &&
                     ((lba_mid == 0x3C && lba_high == 0xC3) ||
                      (lba_mid == 0x69 && lba_high == 0x96)))
            {
                debug(ATA_DRIVER, "IDENTIFY command aborted: SATA device found\n");
            }
            else if (count == 0x01 && lba_low == 0x01 && lba_mid == 0xCE &&
                     lba_high == 0xAA)
            {
                debug(ATA_DRIVER, "IDENTIFY command aborted: Obsolete device identifier\n");
            }
            else
            {
                debug(ATA_DRIVER,
                      "IDENTIFY command aborted: unknown device type identifier\n");
            }
        }
        else
        {
            if (count == 0x01 && lba_low == 0x01 && lba_mid == 0x00 && lba_high == 0x00)
            {
                debug(ATA_DRIVER, "IDENTIFY: PATA device found\n");
            }
            else
            {
                debug(ATA_DRIVER, "IDENTIFY: Command succeeded but signature does not match ATA device\n");
            }
        }
    }
}

IDEController::IDEController(const eastl::string& name) :
    Device(name)
{
    debug(IDE_DRIVER, "Init %s\n", deviceName().c_str());
    doDeviceDetection();
}

void IDEController::doDeviceDetection()
{
    // Assume default io registers
    // Normally detected via PCI bus enumeration
    eastl::array<eastl::tuple<const char*, uint16_t, uint16_t, uint8_t>, 4>
        default_channels = {
            {{"Primary IDE Channel", DefaultPorts::PRIMARY_IO,
              DefaultPorts::PRIMARY_CONTROL, DefaultPorts::PRIMARY_ISA_IRQ},
             {"Secondary IDE Channel", DefaultPorts::SECONDARY_IO,
              DefaultPorts::SECONDARY_CONTROL, DefaultPorts::SECONDARY_ISA_IRQ},
             {"Ternary IDE Channel", DefaultPorts::TERNARY_IO,
              DefaultPorts::TERNARY_CONTROL, DefaultPorts::TERNARY_ISA_IRQ},
             {"Quaternary IDE Channel", DefaultPorts::QUATERNARY_IO,
              DefaultPorts::QUATERNARY_CONTROL, DefaultPorts::QUATERNARY_ISA_IRQ}}};

    for (auto& [name, io_reg_base, control_reg_base, isa_irqnum] : default_channels)
    {
        if (IDEControllerChannel::detectChannel(io_reg_base, control_reg_base))
        {
            auto c =
                new IDEControllerChannel{name, io_reg_base, control_reg_base, isa_irqnum};
            channels.push_back(c);
            addSubDevice(*c);
        }
    }
}
