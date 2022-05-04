#include "ATADriver.h"

#include "BDManager.h"
#include "BDRequest.h"
#include "ArchInterrupts.h"
#include "8259.h"
#include "APIC.h"

#include "Scheduler.h"
#include "kprintf.h"

#include "Thread.h"
#include "offsets.h"

#define TIMEOUT_WARNING() do { kprintfd("%s:%d: timeout. THIS MIGHT CAUSE SERIOUS TROUBLE!\n", __PRETTY_FUNCTION__, __LINE__); } while (0)

#define TIMEOUT_CHECK(CONDITION,BODY) jiffies = 0;\
                                       while((CONDITION) && jiffies++ < IO_TIMEOUT)\
                                         ArchInterrupts::yieldIfIFSet();\
                                       if(jiffies >= IO_TIMEOUT)\
                                       {\
                                         BODY;\
                                       }

ATADriver::ATADriver(uint16 baseport, uint16 getdrive, uint16 irqnum) :
    port(baseport),
    drive((getdrive == 0) ? 0xA0 : 0xB0),
    jiffies(0),
    lock_("ATADriver::lock_")
{
  debug(ATA_DRIVER, "ctor: Entered with irqnum %d and baseport %x!!\n", irqnum, baseport);
  debug(ATA_DRIVER, "ctor: Requesting disk geometry !!\n");

  outportbp (port + 6, drive);  // Get first drive
  outportbp (port + 7, 0xEC);   // Get drive info data
  TIMEOUT_CHECK(inportbp(port + 7) != 0x58,TIMEOUT_WARNING(); return;);

  uint16 dd[256];

  for (uint16 & dd_16 : dd) // Read "sector" 512 b
  {
    dd_16 = inportw(port);
  }

  debug(ATA_DRIVER, "max. original PIO support: %x, PIO3 support: %x, PIO4 support: %x\n", (dd[51] >> 8), (dd[64] & 0x1) != 0, (dd[64] & 0x2) != 0);

  debug(ATA_DRIVER, "ctor: Disk geometry read !!\n");

  HPC = dd[3];
  SPT = dd[6];
  uint32 CYLS = dd[1];
  numsec = CYLS * HPC * SPT;

  debug(ATA_DRIVER, "Enabling interrupts for ATA IRQ check\n");
  {
      WithInterrupts intr(true);
      ArchInterrupts::enableIRQ(irqnum);
      testIRQ();
  }

  irq = irqnum;
  debug(ATA_DRIVER, "ctor: Using ATA mode: %d !!\n", (int)mode);

  debug(ATA_DRIVER, "ctor: Driver created !!\n");
}

void ATADriver::testIRQ( )
{
  mode = BD_ATA_MODE::BD_PIO;

  BDManager::getInstance()->probeIRQ = true;
  readSector( 0, 1, nullptr );

  debug(ATA_DRIVER, "Waiting for ATA IRQ\n");
  TIMEOUT_CHECK(BDManager::getInstance()->probeIRQ,mode = BD_ATA_MODE::BD_PIO_NO_IRQ;);
}

int32 ATADriver::rawReadSector ( uint32 start_sector, uint32 num_sectors, void *buffer )
{
  BD_ATA_MODE old_mode = mode;
  mode = BD_ATA_MODE::BD_PIO_NO_IRQ;
  uint32 result = readSector ( start_sector, num_sectors, buffer );
  mode = old_mode;

  return result;
}

int32 ATADriver::selectSector(uint32 start_sector, uint32 num_sectors)
{
  /* Wait for drive to clear BUSY */
  TIMEOUT_CHECK(inportbp(port + 7) & 0x80,TIMEOUT_WARNING(); return -1;);

  //LBA: linear base address of the block
  //CYL: value of the cylinder CHS coordinate
  //HPC: number of heads per cylinder for the disk
  //HEAD: value of the head CHS coordinate
  //SPT: number of sectors per track for the disk
  //SECT: value of the sector CHS coordinate
  //TEMP: buffer to hold a temporary value

  uint32 LBA = start_sector;
  uint32 cyls = LBA / (HPC * SPT);
  uint32 TEMP = LBA % (HPC * SPT);
  uint32 head = TEMP / SPT;
  uint32 sect = TEMP % SPT + 1;

  uint8 high = cyls >> 8;
  uint8 lo = cyls & 0x00FF;

  //debug(ATA_DRIVER, "readSector:(drive | head): %d, num_sectors: %d, sect: %d, lo: %d, high: %d!!\n",(drive | head),num_sectors,sect,lo,high);

  outportbp(port + 6, (drive | head)); // drive and head selection
  outportbp(port + 2, num_sectors); // number of sectors to read
  outportbp(port + 3, sect); // starting sector
  outportbp(port + 4, lo); // cylinder low
  outportbp(port + 5, high); // cylinder high

  /* Wait for drive to set DRDY */
  TIMEOUT_CHECK((!inportbp(port + 7)) & 0x40,TIMEOUT_WARNING(); return -1;);

  return 0;
}

int32 ATADriver::readSector ( uint32 start_sector, uint32 num_sectors, void *buffer )
{
  if ((ATA_DRIVER & OUTPUT_ENABLED) && (ATA_DRIVER & OUTPUT_ADVANCED))
  {
      debug(ATA_DRIVER, "readSector %x, num: %x into buffer %p\n", start_sector, num_sectors, buffer);
  }

  assert(buffer || (start_sector == 0 && num_sectors == 1));
  if (selectSector(start_sector, num_sectors) != 0)
    return -1;

  for (int i = 0;; ++i)
  {
    /* Write the command code to the command register */
    outportbp(port + 7, 0x20); // command

    if (mode != BD_ATA_MODE::BD_PIO_NO_IRQ)
      return 0;

    jiffies = 0;
    while (inportbp(port + 7) != 0x58 && jiffies++ < IO_TIMEOUT)
      ArchInterrupts::yieldIfIFSet();
    if (jiffies >= IO_TIMEOUT)
    {
      if (i == 3)
      {
        TIMEOUT_WARNING();
        return -1;
      }
    }
    else
      break;
  }

  if (buffer)
  {
    uint32 counter;
    uint16 *word_buff = (uint16 *) buffer;
    for (counter = 0; counter != (256*num_sectors); counter++)  // read sector
        word_buff [counter] = inportw ( port );
  }
  /* Wait for drive to clear BUSY */
  TIMEOUT_CHECK(inportbp(port + 7) & 0x80,TIMEOUT_WARNING(); return -1;);

  //debug(ATA_DRIVER, "readSector:Read successfull !!\n");
  return 0;
}

int32 ATADriver::writeSector ( uint32 start_sector, uint32 num_sectors, void * buffer )
{
  assert(buffer);
  if (selectSector(start_sector, num_sectors) != 0)
    return -1;

  uint16 *word_buff = (uint16 *) buffer;

  /* Write the command code to the command register */
  outportbp( port + 7, 0x30 );           // command

  TIMEOUT_CHECK(inportbp(port + 7) != 0x58,TIMEOUT_WARNING(); return -1;);


  uint32 count2 = (256*num_sectors);
  if( mode != BD_ATA_MODE::BD_PIO_NO_IRQ )
    count2 = 256;

  uint32 counter;
  for (counter = 0; counter != count2; counter++)
      outportw ( port, word_buff [counter] );

  /* Wait for drive to clear BUSY */
  TIMEOUT_CHECK(inportbp(port + 7) & 0x80,TIMEOUT_WARNING(); return -1;);

  /* Write flush code to the command register */
  outportbp (port + 7, 0xE7);

  /* Wait for drive to clear BUSY */
  TIMEOUT_CHECK(inportbp(port + 7) & 0x80,TIMEOUT_WARNING(); return -1;);

  return 0;
}

uint32 ATADriver::addRequest(BDRequest* br)
{
  MutexLock lock(lock_); // this lock might serialize stuff too much...
  bool interrupt_context = false;
  debug(ATA_DRIVER, "addRequest, cmd = %d!\n", (int)br->getCmd());
  if (mode != BD_ATA_MODE::BD_PIO_NO_IRQ)
  {
    interrupt_context = ArchInterrupts::disableInterrupts();

    request_list_.pushFront(br);
  }

  int32 res = -1;

  switch(br->getCmd())
  {
  case BDRequest::BD_CMD::BD_READ:
      res = readSector( br->getStartBlock(), br->getNumBlocks(), br->getBuffer() );
      break;
  case BDRequest::BD_CMD::BD_WRITE:
      res = writeSector( br->getStartBlock(), br->getNumBlocks(), br->getBuffer() );
      break;
  default:
      res = -1;
      break;
  }

  if (res != 0)
  {
    br->setStatus(BDRequest::BD_RESULT::BD_ERROR);
    debug(ATA_DRIVER, "Got out on error !!\n");
    if (interrupt_context)
      ArchInterrupts::enableInterrupts();
    return 0;
  }

  if (mode == BD_ATA_MODE::BD_PIO_NO_IRQ)
  {
    debug(ATA_DRIVER, "addRequest: No IRQ operation !!\n");
    br->setStatus(BDRequest::BD_RESULT::BD_DONE);
    return 0;
  }

  if (currentThread)
  {
    if(interrupt_context)
      ArchInterrupts::enableInterrupts();
    jiffies = 0;
    while (br->getStatus() == BDRequest::BD_RESULT::BD_QUEUED && jiffies++ < IO_TIMEOUT*10);
    if (jiffies >= IO_TIMEOUT*10)
      TIMEOUT_WARNING();
    if (br->getStatus() == BDRequest::BD_RESULT::BD_QUEUED)
    {
      debug(ATA_DRIVER, "addRequest: Request is still pending, going to sleep\n");
      ArchInterrupts::disableInterrupts();
      if (br->getStatus() == BDRequest::BD_RESULT::BD_QUEUED)
      {
        currentThread->setState(Sleeping);
        ArchInterrupts::enableInterrupts();
        Scheduler::instance()->yield(); // this is necessary! setting state to sleep and continuing to run is a BAD idea
        debug(ATA_DRIVER, "addRequest: Woke up after sleeping on request, state = %d\n", (int)br->getStatus());
        assert(br->getStatus() != BDRequest::BD_RESULT::BD_QUEUED && "ATA request still not done after waking up from sleep!");
      }
      ArchInterrupts::enableInterrupts();
    }
  }

  debug(ATA_DRIVER, "addRequest: done\n");
  return 0;
}

void ATADriver::resetController(uint16 controller_port)
{
    if (ATA_DRIVER & OUTPUT_ADVANCED)
      debug(ATA_DRIVER, "Reset controller\n");

    outportbp( controller_port + 0x206, 0x04 );
    outportbp( controller_port + 0x206, 0x00 ); // RESET
}

bool ATADriver::waitForController( bool resetIfFailed = true )
{
  uint32 jiffies = 0;
  while( inportbp( port + 7 ) != 0x58  && jiffies++ < IO_TIMEOUT)
    ArchInterrupts::yieldIfIFSet();

  if(jiffies >= IO_TIMEOUT )
  {
    debug(ATA_DRIVER, "waitForController: controler still not ready\n");
    if( resetIfFailed )
    {
      resetController(port);
    }
    return false;
  }
  return true;
}


void ATADriver::serviceIRQ()
{
  if (mode == BD_ATA_MODE::BD_PIO_NO_IRQ)
    return;

  BDRequest* br = request_list_.peekBack();

  if (br == nullptr)
  {
    debug(ATA_DRIVER, "serviceIRQ: IRQ without request!!\n");
    resetController(port);
    return; // not my interrupt
  }

  Thread* requesting_thread = br->getThread();

  debug(ATA_DRIVER, "serviceIRQ: Found active request!!\n");
  assert(br);

  uint16* word_buff = (uint16*) br->getBuffer();
  uint32 counter;
  uint32 blocks_done = br->getBlocksDone();

  // TODO: target thread may not yet be sleeping (this irq handler and section with disabled interrupts in addRequest may run simultaneously if running on other cpu core)

  if (br->getCmd() == BDRequest::BD_CMD::BD_READ)
  {
      debug(ATA_DRIVER, "serviceIRQ: Handling read request\n");
    // This IRQ handler may be run in the context of any arbitrary thread, so we must not attempt to access userspace
    assert((size_t)word_buff >= USER_BREAK);
    if (!waitForController())
    {
      assert(request_list_.popBack() == br);
      br->setStatus(BDRequest::BD_RESULT::BD_ERROR); // br may be deallocated as soon as status is set to error
      debug(ATA_DRIVER, "serviceIRQ: Error while waiting for controller\n");
      if (requesting_thread)
          requesting_thread->setState(Running);
      return;
    }

    for (counter = blocks_done * 256; counter != (blocks_done + 1) * 256; counter++)
      word_buff[counter] = inportw(port);

    blocks_done++;
    br->setBlocksDone(blocks_done);

    if (blocks_done == br->getNumBlocks())
    {
      assert(request_list_.popBack() == br);
      br->setStatus(BDRequest::BD_RESULT::BD_DONE); // br may be deallocated as soon as status is set to done
      debug(ATA_DRIVER, "serviceIRQ: Read finished\n");
      if (requesting_thread)
          requesting_thread->setState(Running);
    }
  }
  else if (br->getCmd() == BDRequest::BD_CMD::BD_WRITE)
  {
    blocks_done++;
    if (blocks_done == br->getNumBlocks())
    {
      assert(request_list_.popBack() == br);
      debug(ATA_DRIVER, "serviceIRQ: All done!!\n");
      br->setStatus( BDRequest::BD_RESULT::BD_DONE ); // br may be deallocated as soon as status is set to done
      debug(ATA_DRIVER, "serviceIRQ: Waking up thread!!\n");
      if (requesting_thread)
          requesting_thread->setState(Running);
    }
    else
    {
      // This IRQ handler may be run in the context of any arbitrary thread, so we must not attempt to access userspace
      assert((size_t)word_buff >= USER_BREAK);
      if (!waitForController())
      {
        assert(request_list_.popBack() == br);
        br->setStatus(BDRequest::BD_RESULT::BD_ERROR); // br may be deallocated as soon as status is set to error
        if (requesting_thread)
            requesting_thread->setState(Running);
        return;
      }

      for (counter = blocks_done*256; counter != (blocks_done + 1) * 256; counter++ )
        outportw(port, word_buff[counter]);

      br->setBlocksDone(blocks_done);
    }
  }
  else
  {
    blocks_done = br->getNumBlocks();
    assert(request_list_.popBack() == br);
    br->setStatus(BDRequest::BD_RESULT::BD_ERROR); // br may be deallocated as soon as status is set to error
    if (requesting_thread)
        requesting_thread->setState(Running);
  }

  debug(ATA_DRIVER, "serviceIRQ: Request handled!!\n");
}
