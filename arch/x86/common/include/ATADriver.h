#pragma once

#include "BDDriver.h"
#include "BDManager.h"
#include "BDVirtualDevice.h"
#include "Device.h"
#include "DeviceBus.h"
#include "DeviceDriver.h"
#include "IDEDriver.h"
#include "IrqDomain.h"
#include "Mutex.h"
#include "NonBlockingQueue.h"
#include "ports.h"
#include "EASTL/span.h"

class BDRequest;
class IDEControllerChannel;

class ATADrive : public BDDriver, public Device
{
public:
    enum class BD_ATA_MODE
    {
        BD_PIO_NO_IRQ,
        BD_PIO,
        BD_DMA,
        BD_UDMA
    };

    ATADrive(IDEControllerChannel& controller, uint16 drive_num);
    ~ATADrive() override = default;

    /**
     * adds the given request to a list and checkes the type of the
     * request. If it is a read or write request, it is beeing executed,
     * or the function returns otherwise.
     *
     */
    uint32 addRequest(BDRequest* br) override;

    /**
     * sets the current mode to BD_PIO_NO_IRQ while the readSector
     * function is being executed
     *
     */
    int32 rawReadSector(uint32, uint32, void*);

    /**
     * @param 1 sector where it should be started to read
     * @param 2 number of sectors
     * @param 3 buffer where to save all that was read
     *
     */
    int32 readSector(uint32, uint32, void*) override;

    /**
     * @param 1 sector where it should be started to write
     * @param 2 number of sectors
     * @param 3 buffer, which content should be written to the sectors
     *
     */
    int32 writeSector(uint32, uint32, void*) override;

    uint32 getNumSectors() override { return numsec; }

    uint32 getSectorSize() override { return sector_word_size * WORD_SIZE; }

    void serviceIRQ() override;

    /**
     * tests if there is an Interrupt Request waiting
     *
     */
    void testIRQ();

    void printIdentifyInfo(eastl::span<uint16_t, 256> id) const;

    uint32 HPC, SPT; // HEADS PER CYLINDER and SECTORS PER TRACK

protected:
    int32 selectSector(uint32 start_sector, uint32 num_sectors);

    void pioReadData(eastl::span<uint16_t> buffer);
    void pioWriteData(eastl::span<uint16_t> buffer);

private:
    IDEControllerChannel& controller;
    uint8_t drive_num;

    uint32 numsec;
    // 256 * uint16_t = 512 bytes
    uint32 sector_word_size = 256;
    static constexpr size_t WORD_SIZE = sizeof(uint16_t);

    bool lba;
    bool lba_48bit;

    size_t jiffies = 0;

    BD_ATA_MODE mode;

    IrqDomain irq_domain;

    NonBlockingQueue<BDRequest> request_list_;

    Mutex lock_;
};

struct IDEDeviceDescription;

class PATADeviceDriver : public BasicDeviceDriver,
                         public Driver<ATADrive>,
                         public IDEControllerChannel::bus_device_driver_type
{
public:
    PATADeviceDriver();
    ~PATADeviceDriver() override = default;

    static PATADeviceDriver& instance();

    // Check if driver is compatible with device discovered during IDE bus enumeration
    // If yes, create an actual device based on the description
    bool probe(const IDEDeviceDescription&) override;

    static constexpr IDEDeviceDescription::Signature PATA_DRIVE_SIGNATURE{0x01, 0x01,
                                                                          0x00, 0x00};

private:
};
