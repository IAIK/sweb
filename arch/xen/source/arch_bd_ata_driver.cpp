/********************************************************************
*
*    $Id: arch_bd_ata_driver.cpp,v 1.1 2005/09/21 03:33:52 rotho Exp $
*    $Log: arch_bd_ata_driver.cpp,v $
*    Revision 1.2  2005/09/18 20:46:52  nelles
*
*     Committing in .
*
*     Modified Files:
*     	arch/x86/include/arch_bd_ata_driver.h
*     	arch/x86/include/arch_bd_ide_driver.h
*     	arch/x86/include/arch_bd_manager.h
*     	arch/x86/include/arch_bd_request.h
*     	arch/x86/include/arch_bd_virtual_device.h
*     	arch/x86/source/arch_bd_ata_driver.cpp
*     	arch/x86/source/arch_bd_ide_driver.cpp
*     	arch/x86/source/arch_bd_manager.cpp
*     	arch/x86/source/arch_bd_virtual_device.cpp
*     ----------------------------------------------------------------------
*
********************************************************************/
#include "arch_bd_ata_driver.h"
#include "arch_bd_manager.h"

#include "Scheduler.h"

ATADriver::ATADriver( uint16 baseport, uint16 getdrive, uint16 irqnum )
{
  kprintfd("ATADriver::ctor:Entered !!\n");
  
  jiffies = 0;
  port = baseport;
  drive= (getdrive == 0 ? 0xA0 : 0xB0);

  kprintfd("ATADriver::ctor:Requesting disk geometry !!\n");
  
  outbp (port + 6, drive);		// Get first drive
  outbp (port + 7, 0xEC);		// Get drive info data
  while (  inbp(port + 7) != 0x58 && jiffies++ < 50000 ); 

  if( jiffies == 50000 )
  {
      kprintfd("ATADriver::ctor:Timeout while reading the disk !!\n");
      return;
  }

  for (dd_off = 0; dd_off != 256; dd_off++) // Read "sector" 512 b
    dd [dd_off] = inwp ( port );

  kprintfd("ATADriver::ctor:Disk geometry read !!\n");
  
  HPC = dd[3];
  SPT = dd[6];
  uint32 CYLS = dd[1];
  numsec = CYLS * HPC * SPT;
  
  kprintfd("ATADriver::ctor:testing irqs !!\n");
  testIRQ( );
  irq = irqnum;
  kprintfd("ATADriver::ctor:irq tested !!\n");
  kprintfd("ATADriver::ctor:mode: %d !!\n", mode );
  
  request_queue_ = new FiFo< BDRequest * >( 1 );
  
  kprintfd("ATADriver::ctor:Driver created !!\n");
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
    mode = BD_PIO_NO_IRQ;
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
  // This equation is used very often by operating systems such as DOS 
  // (or SWEB) to calculate the CHS values it needs to send to the disk 
  // controller or INT13h in order to read or write data.
  
  uint32 LBA = start_sector;
  uint32 cyls = LBA / (HPC * SPT);
  uint32 TEMP = LBA % (HPC * SPT);
  uint32 head = TEMP / SPT;
  uint32 sect = TEMP % SPT + 1;

  kprintfd("ATADriver::readSector:Sending commands !!\n");
  
  uint8 high = cyls >> 8;
  uint8 lo = cyls & 0x00FF;
  outbp( port + 6, (drive | head) ); // drive and head selection
  outbp( port + 2, num_sectors );	// number of sectors to read
  outbp( port + 3, sect );			// starting sector
  outbp( port + 4, lo );				// cylinder low
  outbp( port + 5, high );			// cylinder high
  outbp( port + 7, 0x20 );			// command
  
  kprintfd("ATADriver::readSector:Commands sent !!\n");
  
  if( mode != BD_PIO_NO_IRQ )
    return 0;
  
  kprintfd("ATADriver::readSector:No IRQ mode !!\n");
  
  jiffies=0;
  while( inbp( port + 7 ) != 0x58  && jiffies++ < 50000);

  if(jiffies > 50000 )
    return -1;

  uint32 counter;
  uint16 * word_buff = (uint16 *) buffer;
  for (counter = 0; counter != (256*num_sectors); counter++)  // read sector
      word_buff [counter] = inwp ( port );
  
  kprintfd("ATADriver::readSector:Read successfull !!\n");          
  return 0;  
}

int32 ATADriver::writeSector ( uint32 start_sector, uint32 num_sectors, void * buffer )
{
	uint16 * word_buff = (uint16 *) buffer;
	
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
  outbp( port + 2, num_sectors );    // number of sectors to read
  outbp( port + 3, sect );           // starting sector
  outbp( port + 4, lo );             // cylinder low
  outbp( port + 5, high );           // cylinder high
  outbp( port + 7, 0x30 );           // command

  jiffies = 0;
  while( inbp( port + 7 ) != 0x58  && jiffies++ < 50000);

  if(jiffies > 50000 )
  {
      kprintfd("ATADriver::writeSector: timeout");
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

uint32 ATADriver::addRequest( BDRequest * br )
{
  kprintfd("ATADriver::addRequest:Adding request !!\n");
  request_queue_->put( br );
  kprintfd("ATADriver::addRequest:Request added !!\n");
  
  int32 res = -1;
  
  switch( br->getCmd() )
  {
    case BDRequest::BD_READ:
      kprintfd("ATADriver::addRequest:Issuing read !!\n");    
      res = readSector( br->getStartBlock(), br->getNumBlocks(), br->getBuffer() );
      break;
    case BDRequest::BD_WRITE:
      kprintfd("ATADriver::addRequest:Issuing write !!\n");        
      res = writeSector( br->getStartBlock(), br->getNumBlocks(), br->getBuffer() );
      break;
    default:
      res = -1;
      break;
  }
  
  if( res != 0 )
  {
    kprintfd("ATADriver::addRequest:Error !!\n");      
    br->setStatus( BDRequest::BD_ERROR );
    return 0;
  }
  
  if( mode == BD_PIO_NO_IRQ )
  {
    kprintfd("ATADriver::addRequest:No IRQ operation !!\n");      
    br->setStatus( BDRequest::BD_DONE );
    request_queue_->get();
    return 0;
  }

  kprintfd("ATADriver::addRequest:Request queued !!\n");
  return 0;
}

void ATADriver::serviceIRQ( void )
{
  kprintfd("ATADriver::serviceIRQ:Entering IRQ handler!!\n");        
  
  BDRequest * br;
  
  if( !request_queue_->peekAhead( br ) )
    return; // not my interrrupt
  
  kprintfd("ATADriver::serviceIRQ:Found active request!!\n");
  
  uint16 * word_buff = (uint16 *) br->getBuffer();
  uint32 counter;
  uint32 blocks_done = br->getBlocksDone();

  
  if( br->getCmd() == BDRequest::BD_READ )
  {
    kprintfd("ATADriver::serviceIRQ:Read request found!!\n");
    
    jiffies = 0;
    while( inbp( port + 7 ) != 0x58  && jiffies++ < 50000)
      ;
  
    if(jiffies > 50000 )
    {
      kprintfd("ATADriver::IRQhandler: controler still not ready\n");
      kprintfd("ATADriver::IRQhandler: reseting\n");
      outbp( port + 0x206, 0x04 ); 
      outbp( port + 0x206, 0x00 ); // RESET
      
      request_queue_->get( );
      br->setStatus( BDRequest::BD_ERROR );
      Scheduler::instance()->wake( br->getThread() );
      
      return;
    }

    for ( counter = blocks_done * 256; 
          counter != (blocks_done + 1) * 256; 
          counter++ )  
        word_buff [counter] = inwp ( port );
    
    blocks_done++;
    br->setBlocksDone( blocks_done );
  }
  else if( br->getCmd() == BDRequest::BD_WRITE )
  {
    kprintfd("ATADriver::serviceIRQ:Write request found!!\n");
    
    uint32 counter;

    for ( counter = blocks_done * 256; 
          counter != (blocks_done + 1) * 256; 
          counter++ )  
       outwp ( port, word_buff [counter] );
            
    blocks_done++;
    br->setBlocksDone( blocks_done );
  }
  else
  {
    blocks_done = br->getNumBlocks();
    kprintfd("ATADriver::IRQHandler:Who changed the universe\n");
  }
  
  kprintfd("ATADriver::serviceIRQ:Request handled!!\n");
  
  if( blocks_done == br->getNumBlocks() )
  {
    kprintfd("ATADriver::serviceIRQ:All done!!\n");
    request_queue_->get( );
    br->setStatus( BDRequest::BD_DONE );
    kprintfd("ATADriver::serviceIRQ:Waking up thread!!\n");
    Scheduler::instance()->wake( br->getThread() );
  }
}
