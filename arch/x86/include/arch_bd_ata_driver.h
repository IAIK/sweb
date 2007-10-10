/**
 * @file arch_bd_ata_driver.h
 *
 */
 
#ifndef _ATA_DEVICE_DRIVER_
#define _ATA_DEVICE_DRIVER_

#include "arch_bd_request.h"
#include "arch_bd_driver.h"
#include "arch_bd_io.h"

class ATADriver : public BDDriver, bdio
{
  public:
  
    typedef enum BD_ATA_MODE_ {
      BD_PIO_NO_IRQ,
      BD_PIO,
      BD_DMA,
      BD_UDMA
    } BD_ATA_MODES;

    /**
     * adds the given request to a list and checkes the type of the
     * request. If it is a read or write request, it is beeing executed,
     * or the function returns otherwise.
     *
     */
    uint32 addRequest( BDRequest * );

    /**
     * Constructor
     *
     */
    ATADriver( uint16 baseport, uint16 getdrive, uint16 irqnum );

    /**
     * Destructor
     *
     */
    virtual ~ATADriver() {};

    /**
     * sets the current mode to BD_PIO_NO_IRQ while the readSector
     * function is being executed
     *
     */
    int32 rawReadSector( uint32, uint32, void * );

    /**
     * @param 1 sector where it should be started to read
     * @param 2 number of sectors
     * @param 3 buffer where to save all that was read
     *
     */
    int32 readSector( uint32, uint32, void * );

    /**
     * @param 1 sector where it should be started to write
     * @param 2 number of sectors
     * @param 3 buffer, which content should be written to the sectors
     *
     */
    int32 writeSector( uint32, uint32, void * );

    /**
     * @return number of sectors
     *
     */
    uint32 getNumSectors() { return numsec; };

    /**
     * @return size of a sector
     *
     */
    uint32 getSectorSize() { return 512; };

    /**
     * handles the active requests
     *
     */
    void serviceIRQ();

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
    bool waitForController( bool resetIfFailed );

    uint32 HPC, SPT;    // HEADS PER CYLINDER and SECTORS PER TRACK

  private:

    uint16 dd [256];    // read buffer if we need one
    uint32 dd_off;      // read buffer counter

    uint32 numsec;

    uint16 port;
    uint16 drive;

    uint32 jiffies;

    BD_ATA_MODES mode; // mode see enum BD_ATA_MODES

    BDRequest *request_list_;
    BDRequest *request_list_tail_;
};

#endif
