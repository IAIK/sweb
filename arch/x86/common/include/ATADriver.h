#pragma once

#include "BDDriver.h"
#include "Mutex.h"
#include "NonBlockingQueue.h"
#include "IrqDomain.h"

class BDRequest;

class ATADriver : public BDDriver
{
  public:

    enum class BD_ATA_MODE
    {
      BD_PIO_NO_IRQ,
      BD_PIO,
      BD_DMA,
      BD_UDMA
    };

    /**
     * adds the given request to a list and checkes the type of the
     * request. If it is a read or write request, it is beeing executed,
     * or the function returns otherwise.
     *
     */
    uint32 addRequest(BDRequest* br) override;
    ATADriver(uint16 baseport, uint16 getdrive, uint16 irqnum);
    ~ATADriver() override = default;

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
      return 512;
    }

    void serviceIRQ() override;

    /**
     * tests if there is an Interrupt Request waiting
     *
     */
    void testIRQ();

    /**
     * tests if the Controller is available
     * false if it is not or the time is elapsed
     *
     */
    bool waitForController(bool resetIfFailed);

    uint32 HPC, SPT; // HEADS PER CYLINDER and SECTORS PER TRACK


  private:

    static void resetController(uint16 port);

    int32 selectSector(uint32 start_sector, uint32 num_sectors);

    uint32 numsec;

    uint16 port;
    uint16 drive;

    uint32 jiffies;

    BD_ATA_MODE mode;

    IrqDomain irq_domain;

    NonBlockingQueue<BDRequest> request_list_;

    Mutex lock_;
};
