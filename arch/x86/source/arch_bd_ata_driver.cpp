/**
 * @file arch_bd_ata_driver.cpp
 *
 */
 
#include "arch_bd_ata_driver.h"
#include "arch_bd_manager.h"

#include "ArchInterrupts.h"
#include "8259.h"

#include "Scheduler.h"
#include "kprintf.h"

ATADriver::ATADriver( uint16 baseport, uint16 getdrive, uint16 irqnum )
{
  debug(ATA_DRIVER, "ctor: Entered with irgnum %d and baseport %d!!\n", irqnum, baseport);

  jiffies = 0;
  port = baseport;
  drive= (getdrive == 0 ? 0xA0 : 0xB0);

  debug(ATA_DRIVER, "ctor: Requesting disk geometry !!\n");

  outbp (port + 6, drive);  // Get first drive
  outbp (port + 7, 0xEC);   // Get drive info data
  while (  inbp(port + 7) != 0x58 && jiffies++ < 50000 ); 

  if( jiffies == 50000 )
  {
    debug(ATA_DRIVER, "ctor: Timeout while reading the disk !!\n");
      return;
  }

  for (dd_off = 0; dd_off != 256; dd_off++) // Read "sector" 512 b
    dd [dd_off] = inwp ( port );

  debug(ATA_DRIVER, "ctor: Disk geometry read !!\n");

  HPC = dd[3];
  SPT = dd[6];
  uint32 CYLS = dd[1];
  numsec = CYLS * HPC * SPT;

  bool interrupt_context = ArchInterrupts::disableInterrupts();
  ArchInterrupts::enableInterrupts();

  enableIRQ( irqnum );
  if( irqnum > 8 )
  {
    enableIRQ( 2 );   // cascade
  }

  testIRQ( );
  if( !interrupt_context )
    ArchInterrupts::disableInterrupts();
  irq = irqnum;
  debug(ATA_DRIVER, "ctor: mode: %d !!\n", mode );

  request_list_ = 0;
  request_list_tail_ = 0;

  debug(ATA_DRIVER, "ctor: Driver created !!\n");
  return;
}

void ATADriver::testIRQ( )
{
  mode = BD_PIO;

  BDManager::getInstance()->probeIRQ = true;
  readSector( 0, 1, 0 );

  jiffies = 0;
  while( BDManager::getInstance()->probeIRQ && jiffies++ < 50000 );

  if( jiffies > 50000 )
  {
    mode = BD_PIO_NO_IRQ; 
  }
}

int32 ATADriver::rawReadSector ( uint32 start_sector, uint32 num_sectors, void *buffer )
{
  BD_ATA_MODES old_mode = mode;
  mode = BD_PIO_NO_IRQ;
  uint32 result = readSector ( start_sector, num_sectors, buffer );
  mode = old_mode;

  return result;
}

int32 ATADriver::readSector ( uint32 start_sector, uint32 num_sectors, void *buffer )
{
  //The equations to convert from LBA to CHS follow:
  //CYL = LBA / (HPC * SPT)
  //TEMP = LBA % (HPC * SPT)
  //HEAD = TEMP / SPT
  //SECT = TEMP % SPT + 1
  //Where:
  //LBA: linear base address of the block
  //CYL: value of the cylinder CHS coordinate
  //HPC: number of heads per cylinder for the disk
  //HEAD: value of the head CHS coordinate
  //SPT: number of sectors per track for the disk
  //SECT: value of the sector CHS coordinate
  //TEMP: buffer to hold a temporary value
  //
  // This equation is used very often by operating systems such as DOS 
  // (or SWEB) to calculate the CHS values it needs to send to the disk 
  // controller or INT13h in order to read or write data.

  uint32 LBA = start_sector;
  uint32 cyls = LBA / (HPC * SPT);
  uint32 TEMP = LBA % (HPC * SPT);
  uint32 head = TEMP / SPT;
  uint32 sect = TEMP % SPT + 1;

  uint8 high = cyls >> 8;
  uint8 lo = cyls & 0x00FF;

  //debug(ATA_DRIVER, "readSector:(drive | head): %d, num_sectors: %d, sect: %d, lo: %d, high: %d!!\n",(drive | head),num_sectors,sect,lo,high);

  outbp( port + 6, (drive | head) );  // drive and head selection
  outbp( port + 2, num_sectors );     // number of sectors to read
  outbp( port + 3, sect );            // starting sector
  outbp( port + 4, lo );              // cylinder low
  outbp( port + 5, high );            // cylinder high
  outbp( port + 7, 0x20 );            // command

  if( mode != BD_PIO_NO_IRQ )
    return 0;

  jiffies = 0;

  while( inbp( port + 7 ) != 0x58  && jiffies++ < 50000);
    if(jiffies > 50000 )
      return -1;

  uint32 counter;
  uint16 *word_buff = (uint16 *) buffer;
  for (counter = 0; counter != (256*num_sectors); counter++)  // read sector
      word_buff [counter] = inwp ( port );

  //debug(ATA_DRIVER, "readSector:Read successfull !!\n");
  return 0;  
}

int32 ATADriver::writeSector ( uint32 start_sector, uint32 num_sectors, void * buffer )
{
  uint16 *word_buff = (uint16 *) buffer;

  // This equation is used very often by operating systems such as DOS 
  // (or SWEB) to calculate the CHS values it needs to send to the disk 
  // controller or INT13h in order to read or write data.

  uint32 LBA = start_sector;
  uint32 cyls = LBA / (HPC * SPT);
  uint32 TEMP = LBA % (HPC * SPT);
  uint32 head = TEMP / SPT;
  uint32 sect = TEMP % SPT + 1;


  uint8 high = cyls >> 8;
  uint8 lo = cyls & 0x00FF;
  outbp( port + 6, (drive | head) ); // drive and head selection
  outbp( port + 2, num_sectors );    // number of sectors to write
  outbp( port + 3, sect );           // starting sector
  outbp( port + 4, lo );             // cylinder low
  outbp( port + 5, high );           // cylinder high
  outbp( port + 7, 0x30 );           // command

  jiffies = 0;
  while( inbp( port + 7 ) != 0x58  && jiffies++ < 50000);

  if(jiffies > 50000 )
  {
    debug(ATA_DRIVER, "writeSector: timeout");
      return -1;
  }

  uint32 count2 = (256*num_sectors);
  if( mode != BD_PIO_NO_IRQ )
    count2 = 256;

  uint32 counter;
  for (counter = 0; counter != count2; counter++) 
      outwp ( port, word_buff [counter] );

  return 0;
}

uint32 ATADriver::addRequest( BDRequest *br )
{
  bool interrupt_context = false;
  debug(ATA_DRIVER, "addRequest %d!\n", br->getCmd() );
  if( mode != BD_PIO_NO_IRQ )
  {
    interrupt_context = ArchInterrupts::disableInterrupts();

    //Add request to the list protected by the cli
    if( request_list_ == 0 )
      request_list_ = request_list_tail_ = br;
    else
    {
      request_list_tail_->setNextRequest(br) ;
      request_list_tail_ = br;
    }
  }

  int32 res = -1;
  
  switch( br->getCmd() )
  {
    case BDRequest::BD_READ:
      res = readSector( br->getStartBlock(), br->getNumBlocks(), br->getBuffer() );
      break;
    case BDRequest::BD_WRITE:
      res = writeSector( br->getStartBlock(), br->getNumBlocks(), br->getBuffer() );
      break;
    default:
      res = -1;
      break;
  }
  
  if( res != 0 )
  {
    br->setStatus( BDRequest::BD_ERROR );
    debug(ATA_DRIVER, "Got out on error !!\n");
      if( interrupt_context )
        ArchInterrupts::enableInterrupts();
    return 0;
  }

  if( mode == BD_PIO_NO_IRQ )
  {
    debug(ATA_DRIVER, "addRequest:No IRQ operation !!\n");
    br->setStatus( BDRequest::BD_DONE );
    return 0;
  }

  if( currentThread )
    Scheduler::instance()->sleepAndRestoreInterrupts(interrupt_context);

  return 0;
}

bool ATADriver::waitForController( bool resetIfFailed = true )
{
  uint32 jiffies = 0;
  while( inbp( port + 7 ) != 0x58  && jiffies++ < 50000)
    ;

  if(jiffies > 50000 )
  {
    debug(ATA_DRIVER, "waitForController: controler still not ready\n");
    if( resetIfFailed )
    {
      debug(ATA_DRIVER, "waitForController: reseting\n");
      outbp( port + 0x206, 0x04 );
      outbp( port + 0x206, 0x00 ); // RESET
    }
    return false;
  }
  return true;
}

void ATADriver::serviceIRQ()
{
  if( mode == BD_PIO_NO_IRQ )
    return;

  if( request_list_ == 0 )
  {
    debug(ATA_DRIVER, "serviceIRQ: IRQ without request!!\n");
    outbp( port + 0x206, 0x04 );
    outbp( port + 0x206, 0x00 ); // RESET COTROLLER
    debug(ATA_DRIVER, "serviceIRQ: Reset controller!!\n");
    return; // not my interrupt
  }

  BDRequest *br = request_list_;
  debug(ATA_DRIVER, "serviceIRQ: Found active request!!\n");

  uint16 *word_buff = (uint16 *) br->getBuffer();
  uint32 counter;
  uint32 blocks_done = br->getBlocksDone();

  if( br->getCmd() == BDRequest::BD_READ )
  {
    if( !waitForController() )
    {
      br->setStatus( BDRequest::BD_ERROR );
      if( br->getThread() )		
        Scheduler::instance()->wake( br->getThread() );
      request_list_ = br->getNextRequest();
      return;
    }

    for(counter = blocks_done * 256; counter!=(blocks_done + 1) * 256; counter++ )
      word_buff [counter] = inwp ( port );

    blocks_done++;
    br->setBlocksDone( blocks_done );

    if( blocks_done == br->getNumBlocks() )
    {
      br->setStatus( BDRequest::BD_DONE );
      request_list_ = br->getNextRequest();
      if( br->getThread() )
        Scheduler::instance()->wake( br->getThread() );
    }
  }
  else if( br->getCmd() == BDRequest::BD_WRITE )
  {
    blocks_done++;
    if( blocks_done == br->getNumBlocks() )
    {
      debug(ATA_DRIVER, "serviceIRQ:All done!!\n");
      br->setStatus( BDRequest::BD_DONE );
      debug(ATA_DRIVER, "serviceIRQ:Waking up thread!!\n");
      request_list_ = br->getNextRequest();

      if( br->getThread() )
        Scheduler::instance()->wake( br->getThread() );

    }
    else
    {
      if( !waitForController() )
      {
        br->setStatus( BDRequest::BD_ERROR );
        if( br->getThread() )
           Scheduler::instance()->wake( br->getThread() );
        request_list_ = br->getNextRequest();
        return;
      }
	
      for(counter = blocks_done*256; counter != (blocks_done + 1) * 256; counter++ )
        outwp ( port, word_buff [counter] );

      br->setBlocksDone( blocks_done );
    }
  }
  else
  {
    blocks_done = br->getNumBlocks();
    br->setStatus( BDRequest::BD_ERROR );
    request_list_ = br->getNextRequest();
    if( br->getThread() )
      Scheduler::instance()->wake( br->getThread() );
  }

  debug(ATA_DRIVER, "serviceIRQ:Request handled!!\n");
}
