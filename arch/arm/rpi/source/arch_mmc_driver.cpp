/**
 * @file arch_mmc_driver.cpp
 *
 */
 
#include "arch_mmc_driver.h"
#include "arch_bd_manager.h"
#include "arch_bd_request.h"

#include "ArchInterrupts.h"

#include "Scheduler.h"
#include "kprintf.h"

struct MMCI* mmci = (struct MMCI*) 0x8C000000;
uint32* mmci_fifo = (uint32*) 0x8C000080;

uint32 mmc_send_cmd(uint32 command, uint32 arg, uint32* response, uint32 write = 1, uint32 data = 0)
{
//  kprintfd("---> command = %d, arg = %x\n",command,arg);
  mmci->arg1 = arg;
  mmci->cmdtm = ((command & 0x3F) << 24) | (data ? (1 << 21) : 0) | (response ? (1 << 16) : 0) | (write ? 0 : (1 << 4));

//  kprintfd("status: %x\n",mmci->status);
//  kprintfd("resp0: %x\n",mmci->resp0);
//  kprintfd("resp1: %x\n",mmci->resp1);
//  kprintfd("resp2: %x\n",mmci->resp2);
//  kprintfd("resp3: %x\n",mmci->resp3);
  if (response)
    *response = mmci->resp0;
  //mmci->clear = mmci->status & 0x1FF;
  return mmci->status;
}

uint32 mmc_send_acmd(uint32 command, uint32 arg, uint32* response)
{
  uint32 first_response;
  mmc_send_cmd(55, 0, &first_response);
  return mmc_send_cmd(command, arg, response);
}

MMCDriver::MMCDriver() : SPT(63), lock_("MMCDriver::lock_"), rca_(0), sector_size_(512), num_sectors_(210672)
{
  debug(MMC_DRIVER,"MMCDriver()\n");
  uint32 response;
  // protocol from sd card specification
  mmc_send_cmd(0,0,0); // go to idle state
  mmc_send_acmd(41,0xffff00ff,&response); // get ocr register 01 101001 0 0 0 1 000 0 1111111111
  assert(response == 0x80ffff00);
  mmc_send_cmd(2,0,0);
  mmc_send_cmd(3,0,&response);
  rca_ = response >> 16;
  mmc_send_cmd(7,rca_ << 16,0);
}

MMCDriver::~MMCDriver()
{

}

uint32 MMCDriver::addRequest( BDRequest * br)
{
  MutexLock lock(lock_);
  debug(MMC_DRIVER, "addRequest %d!\n", br->getCmd() );

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

  debug(MMC_DRIVER, "addRequest:No IRQ operation !!\n");
  br->setStatus( BDRequest::BD_DONE );
  return res;
}

int32 MMCDriver::readBlock ( uint32 address, void *buffer )
{
  debug(MMC_DRIVER,"readBlock: address: %x, buffer: %x\n",address, buffer);
  uint32 response;
  mmci->blksizecnt = (1 << 16) | 512;
  mmc_send_cmd(17,address,&response,0,1);
  uint32* buffer32 = (uint32*) buffer;
  uint32 i = 0;
  uint32 temp;
  while (i < (mmci->blksizecnt & 0x3FF) / sizeof(uint32))
  {
    // we should check whether there is something to read...
    buffer32[i++] = mmci->data;
  }
  return 0;
}

int32 MMCDriver::readSector ( uint32 start_sector, uint32 num_sectors, void *buffer )
{
  debug(MMC_DRIVER,"readSector: start: %x, num: %x, buffer: %x\n",start_sector, num_sectors, buffer);
  for (uint32 i = 0; i < num_sectors; ++i)
  {
    readBlock((start_sector + i) * sector_size_, (char*)buffer + i * sector_size_);
  }
  return 0;
}

int32 MMCDriver::writeBlock ( uint32 address, void *buffer )
{
  return 0;
}

int32 MMCDriver::writeSector ( uint32 start_sector, uint32 num_sectors, void * buffer  )
{
  while(1);
  return 0;
}

uint32 MMCDriver::getNumSectors()
{
  return num_sectors_;
}

uint32 MMCDriver::getSectorSize()
{
  return sector_size_;
}

void MMCDriver::serviceIRQ()
{
}
