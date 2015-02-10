/**
 * @file arch_mmc_driver.cpp
 *
 */
 
#include "BDManager.h"
#include "BDRequest.h"
#include "MMCDriver.h"
#include "ArchInterrupts.h"

#include "Scheduler.h"
#include "kprintf.h"

#define TIMEOUT_WARNING() do { kprintfd("%s:%d: timeout. THIS MIGHT CAUSE SERIOUS TROUBLE!\n", __PRETTY_FUNCTION__, __LINE__); } while (0)

struct MMCI
{
  uint32 strpcl;
  uint32 stat;
  uint32 clkrt;
  uint32 spi;
  uint32 cmdat;
  uint32 rest0;
  uint32 rdt0;
  uint32 blklen;
  uint32 numblk;
  uint32 prtbuf;
  uint32 imask;
  uint32 ireg;
  uint32 cmd;
  uint32 argh;
  uint32 argl;
  uint32 res;
  uint32 rxfifo;
  uint32 txfifo;
  uint32 rdwait;
  uint32 blks_rem;
};

struct MMCI* mmci = (struct MMCI*) 0x8C000000;

uint32 mmc_send_cmd(uint32 command, uint32 arg, uint32* response, uint32 write = 1, uint32 data = 0)
{
//  kprintfd("---> command = %d, arg = %x\n",command,arg);
  mmci->cmd = command & 0xFFFF;
  mmci->argh = (arg >> 16) & 0xFFFF;
  mmci->argl = arg & 0xFFFF;
  mmci->clkrt = 0;
  mmci->spi = 0;
  mmci->rest0 = 0x7F;
  mmci->cmdat = (write ? (1 << 3) : 0) | (response ? (1 << 0) : 0) | (data ? (1 << 2) : 0);
//  #define BIT(X,Y) ((mmci->stat & (1 << X)) ? Y : "")
//    kprintfd("stats: %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s\n",
//             BIT(0,"TimeOutRead"),
//             BIT(1,"TimeOutRes"),
//             BIT(2,"CrcWrErr"),
//             BIT(3,"CrcRdErr"),
//             BIT(4,"DatErrToken"),
//             BIT(5,"ResCrcErr"),
//             BIT(6,"RES"),
//             BIT(7,"RES"),
//             BIT(8,"ClkEn"),
//             BIT(9,"FlashErr"),
//             BIT(10,"SpiWrErr"),
//             BIT(11,"DataTranDone"),
//             BIT(12,"PrgDone"),
//             BIT(12,"EndCmdRes"),
//             BIT(13,"RdStalled"),
//             BIT(14,"SdioInt"),
//             BIT(15,"SdioSuspendAck"));
//  kprintfd("mmci->stat = %x\n",mmci->stat);
  if (response)
  {
    *response = 0;
    *response |= (mmci->res & 0xFF) << 24;
    *response |= (mmci->res & 0xFFFF) << 8;
    *response |= mmci->res & 0xFF;
    *response |= mmci->res & 0xFF;
  } // this response construction is voodoo, but it works. pl181 code is prettier
  return mmci->stat;
}

uint32 mmc_send_acmd(uint32 command, uint32 arg, uint32* response)
{
  uint32 first_response;
  mmc_send_cmd(55, 0, &first_response);
  return mmc_send_cmd(command, arg, response);
}

MMCDriver::MMCDriver() : SPT(63), lock_("MMCDriver::lock_"), rca_(0), sector_size_(512)
{
  irq = 0;
  num_sectors_ = 210672; // Todo
  debug(MMC_DRIVER,"MMCDriver()\n");
  uint32 response;
  // protocol from sd card specification
  mmc_send_cmd(0,0,0); // go to idle state
  mmc_send_acmd(41,0xffff00ff,&response); // get ocr register 01 101001 0 0 0 1 000 0 1111111111
  assert(response == 0x80ffff00);
  mmc_send_cmd(2,0,0);
  mmc_send_cmd(3,0,&response);
  rca_ = response;
  mmc_send_cmd(7,rca_,0);
}

MMCDriver::~MMCDriver()
{

}

uint32 MMCDriver::addRequest( BDRequest * br)
{
  MutexLock lock(lock_);
  debug(MMC_DRIVER, "addRequest %d!\n", br->getCmd() );

  int32 res = -1; if (res){}; // I think we will use this variable in future

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

  debug(MMC_DRIVER, "addRequest:No IRQ operation !!\n");
  br->setStatus( BDRequest::BD_DONE );
  return 0;
}

int32 MMCDriver::readBlock ( uint32 address, void *buffer )
{
  debug(MMC_DRIVER,"readBlock: address: %x, buffer: %x\n",address, buffer);
  uint32 response;
  mmci->blklen = 512;
  mmci->numblk = 1;
  mmci->rdt0 = 0xFFFF;
  mmc_send_cmd(17,address,&response,0,1);
  uint32* buffer32 = (uint32*) buffer;
  uint32 i = 0;
  uint32 temp;
  // actually we should check this according to the manual, but it misses the last 28 bytes!: (!(mmci->stat & (1 << 11))) || (mmci->ireg & (1 << 5)))
  while (i < mmci->blklen / sizeof(uint32))
  {
    temp = mmci->rxfifo;
    buffer32[i++] = ((temp & 0xFF) << 24) | ((temp & 0xFF00) << 8) | ((temp & 0xFF0000) >> 8) | ((temp & 0xFF000000) >> 24);
  }
  return 0;
}

int32 MMCDriver::readSector ( uint32 start_sector, uint32 num_sectors, void *buffer )
{
  debug(MMC_DRIVER,"readSector: start: %x, num: %x, buffer: %x\n",start_sector, num_sectors, buffer);
  for (uint32 i = 0; i < num_sectors; ++i)
  {
    readBlock((start_sector + i) * sector_size_, (void*)((size_t)buffer + i * sector_size_));
  }
  return 0;
}

int32 MMCDriver::writeBlock ( uint32 address, void *buffer)
{
  debug(MMC_DRIVER,"writeBlock: address: %x, buffer: %x\n",address, buffer);
  uint32 response;
  mmci->blklen = 512;
  mmci->numblk = 1;
  mmc_send_cmd(24, address, &response, 1, 1);
  uint32* buffer32 = (uint32*) buffer;
  uint32 i = 0;
  uint32 temp;
  // actually we should check this according to the manual, but it misses the last 28 bytes!: (!(mmci->stat & (1 << 11))) || (mmci->ireg & (1 << 5)))
  while (i < mmci->blklen / sizeof(uint32))
  {
    temp = ((buffer32[i] & 0xFF) << 24) | ((buffer32[i] & 0xFF00) << 8) | ((buffer32[i] & 0xFF0000) >> 8) | ((buffer32[i] & 0xFF000000) >> 24);
    mmci->txfifo = temp;
    i++;
  }
  return 0;
}

int32 MMCDriver::writeSector ( uint32 start_sector, uint32 num_sectors, void * buffer)
{
  debug(MMC_DRIVER,"writeSector: start: %x, num: %x, buffer: %x\n",start_sector, num_sectors, buffer);
  for (uint32 i = 0; i < num_sectors; ++i)
  {
    writeBlock((start_sector + i) * sector_size_, (void*)((size_t)buffer + i * sector_size_));
  }
  return 0;
}

uint32 MMCDriver::getNumSectors()
{
  return 210672; // TODO
}

uint32 MMCDriver::getSectorSize()
{
  return sector_size_;
}

void MMCDriver::serviceIRQ()
{
}
