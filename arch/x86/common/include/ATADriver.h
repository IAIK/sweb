#pragma once

#include "BDDriver.h"
#include "BDManager.h"
#include "BDVirtualDevice.h"
#include "Device.h"
#include "DeviceBus.h"
#include "IrqDomain.h"
#include "Mutex.h"
#include "NonBlockingQueue.h"
#include "ports.h"
#include "EASTL/span.h"

class BDRequest;
class IDEControllerChannel;

class ATADriver : public BDDriver, public Device
{
  public:

    enum class BD_ATA_MODE
    {
      BD_PIO_NO_IRQ,
      BD_PIO,
      BD_DMA,
      BD_UDMA
    };

    struct COMMAND
    {
        struct OTHER
        {
            enum
            {
                NOP = 0x00,
                SET_MULTIPLE_MODE = 0xC6,
                FLUSH_CACHE = 0xE7,
                SET_FEATURES = 0xEF,
            };
        };

        struct PIO
        {
            enum
            {
                READ_SECTORS = 0x20,
                WRITE_SECTORS = 0x30,
                READ_MULTIPLE = 0xC4,
                IDENTIFY_DEVICE = 0xEC,
            };
        };

        struct DMA
        {
            enum
            {
                READ = 0xC8,
                WRITE = 0xCA,
            };
        };

        // 48-bit commands
        struct EXT
        {
            struct OTHER
            {
                enum
                {
                    DATA_SET_MANAGEMENT = 0x06,
                    CONFIGURE_STREAM = 0x51,
                    SET_DATE_TIME = 0x77,
                };
            };

            struct PIO
            {
                enum
                {
                    READ_SECTORS = 0x24,
                    READ_MULTIPLE = 0x29,
                    READ_STREAM = 0x2B,
                    READ_LOG = 0x2F,
                };
            };

            struct DMA
            {
                enum
                {
                    READ = 0x25,
                    WRITE = 0x35,
                    READ_LOG = 0x47,
                    READ_STREAM = 0x2A,
                };
            };
        };
    };

    ATADriver(IDEControllerChannel& controller, uint16 drive_num, eastl::span<uint16_t, 256> identify_data);
    ~ATADriver() override = default;

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
    int32 rawReadSector(uint32, uint32, void *);

    /**
     * @param 1 sector where it should be started to read
     * @param 2 number of sectors
     * @param 3 buffer where to save all that was read
     *
     */
    int32 readSector(uint32, uint32, void *) override;

    /**
     * @param 1 sector where it should be started to write
     * @param 2 number of sectors
     * @param 3 buffer, which content should be written to the sectors
     *
     */
    int32 writeSector(uint32, uint32, void *) override;

    uint32 getNumSectors() override
    {
      return numsec;
    }

    uint32 getSectorSize() override
    {
      return sector_word_size * 2;
    }

    void serviceIRQ() override;

    /**
     * tests if there is an Interrupt Request waiting
     *
     */
    void testIRQ();

    void printIdentifyInfo(eastl::span<uint16_t, 256> id);

    uint32 HPC, SPT; // HEADS PER CYLINDER and SECTORS PER TRACK

protected:
    int32 selectSector(uint32 start_sector, uint32 num_sectors);

    void pioReadData(eastl::span<uint16_t> buffer);
    void pioWriteData(eastl::span<uint16_t> buffer);

private:

    IDEControllerChannel& controller;
    uint8_t drive_num;

    uint32 numsec;
    uint32 sector_word_size = 256; // 256 * uint16_t = 512 bytes

    bool lba;
    bool lba_48bit;

    uint32 jiffies;

    BD_ATA_MODE mode;

    IrqDomain irq_domain;

    NonBlockingQueue<BDRequest> request_list_;

    Mutex lock_;
};
