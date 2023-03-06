#include "MMCDriver.h"
#include "BDManager.h"
#include "BDRequest.h"
#include "BDVirtualDevice.h"
#include "ArchInterrupts.h"
#include "Scheduler.h"
#include "kprintf.h"
#include "MasterBootRecord.h"


struct MMCI {
    uint32_t arg2;
    uint32_t blksizecnt;
    uint32_t arg1;
    uint32_t cmdtm;
    uint32_t resp0;
    uint32_t resp1;
    uint32_t resp2;
    uint32_t resp3;
    uint32_t data;
    uint32_t status;
    uint32_t control0;
    uint32_t control1;
    uint32_t interrupt;
    uint32_t irpt_mask;
    uint32_t irpt_en;
    uint32_t control2;
    uint32_t cap0;
    uint32_t cap1;
    uint32_t rsvd1;
    uint32_t rsvd2;
    uint32 force_irpt;
    uint8_t rsvd3[0x1c];
    uint32_t boot_timeout;
    uint32_t dbg_sel;
    uint32_t rsvd4;
    uint32_t rsvd5;
    uint32_t exrdfifo_cfg;
    uint32_t exrdfifo_en;
    uint32_t tune_step;
    uint32_t tune_steps_std;
    uint32_t tune_steps_ddr;
    uint8_t rsvd6[80];
    uint32_t spi_int_spt;
    uint32_t rsvd7[2];
    uint32_t slotisr_ver;
}__attribute__((packed, aligned(4)));

struct MMCI* mmci = (struct MMCI*) 0x8C000000;

uint32_t mmc_send_cmd(uint32_t command, uint32_t arg, uint32_t* response, uint32_t data = 0)
{
  while(mmci->status & 0x1);

  mmci->arg1 = arg;
  mmci->cmdtm = ((command & 0x3F) << 24) | (response ? (2 << 16) : 0) | (1 << 4) | (data ? (1 << 21) : 0);

  for (uint32 i = 0; i < 0x10000; ++i);

  if (command == 41)
    for (uint32 i = 0; i < 0x100000; ++i);

  while(!(mmci->status & 0x1) && !(mmci->interrupt & 0x1));
//  uint32 temp = 0;
//  kprintfd("interrupt: %x\n",mmci->interrupt);
  mmci->interrupt = 1;
//  kprintfd("status: %x\n",mmci->status);
//  kprintfd("resp0: %x\n",mmci->resp0);
//  kprintfd("resp1: %x\n",mmci->resp1);
//  kprintfd("resp2: %x\n",mmci->resp2);
//  kprintfd("resp3: %x\n",mmci->resp3);

  if (response)
    *response = mmci->resp0;
  return mmci->status;
}

uint32_t mmc_send_acmd(uint32_t command, uint32_t arg, uint32_t* response)
{
  do
  {
    mmc_send_cmd(55, 0, response);
    *response = 0;
    mmc_send_cmd(command,arg,response);
  }
  while (!((*response) & (1 << 31)));
  return 0;
}


MMCDrive::MMCDrive() :
    Device(eastl::string("MMC disk")),
    SPT(63),
    lock_("MMCDriver::lock_"),
    rca_(0),
    sector_size_(512),
    num_sectors_(210672)
{
//  unsigned int check;
  debug(MMC_DRIVER,"MMCDriver()\n");
  uint32_t response;

  uint32_t ver = mmci->slotisr_ver;
  uint32_t vendor = ver >> 24;
  uint32_t sdversion = (ver >> 16) & 0xff;
  uint32_t slot_status = ver & 0xff;
  debug(MMC_DRIVER, "EMMC: vendor %x, sdversion %x, slot_status %x\n", vendor, sdversion, slot_status);

  // Read the capabilities registers
  uint32_t capabilities_0 = mmci->cap0;
  uint32_t capabilities_1 = mmci->cap1;
  debug(MMC_DRIVER, "EMMC: capabilities: %x %x\n", capabilities_0, capabilities_1);

  uint32_t status_reg = mmci->status;
  if((status_reg & (1 << 16)) == 0)
  {
    debug(MMC_DRIVER, "EMMC: no card inserted\n");
  }
  debug(MMC_DRIVER, "EMMC: status: %08x\n", status_reg);

  uint32_t base_clock = ((capabilities_0 >> 8) & 0xff) * 1000000;
  debug(MMC_DRIVER, "EMMC: base clock: %d\n", base_clock);

  mmci->control1 = (1 << 24);
  while (mmci->control1 & (1 << 24));

  mmci->control1 = 0xF0F27; // reset everything, highest clock divider, no timeout
  mmci->control2 = 0x0;

  mmci->blksizecnt = (1 << 16) | sector_size_;
  mmci->irpt_mask = 0xFFFFFFFF;
  while (!(mmci->control1 & (1 << 1)));
  debug(MMC_DRIVER,"MMC controller resetted\n");
  // protocol from sd card specification
  mmc_send_cmd(0,0,0); // go to idle state
  if(sdversion >= 2) mmc_send_cmd(8,0x1AA,&response,1 << 16); // only needed vor SDHC >= 2
  mmc_send_acmd(41,0x50FF0000,&response); // init initialization
  assert((response & 0x80ff8000) == 0x80ff8000);
  mmc_send_cmd(2,0,0);
  mmc_send_cmd(3,0,&response);
  rca_ = response >> 16;
  mmc_send_cmd(7,rca_ << 16,0);

  mmci->irpt_en = 0;

  mmci->irpt_mask = 0xFFFFFFFF;
}

MMCDrive::~MMCDrive()
{

}

uint32_t MMCDrive::addRequest( BDRequest * br)
{
  ScopeLock lock(lock_);
  debug(MMC_DRIVER, "addRequest %d!\n", (int)br->getCmd());

  int32_t res = -1;

  switch( br->getCmd() )
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

  debug(MMC_DRIVER, "addRequest:No IRQ operation !!\n");
  br->setStatus( BDRequest::BD_RESULT::BD_DONE );
  return res;
}

int32_t MMCDrive::readBlock ( uint32_t address, void *buffer )
{
  debug(MMC_DRIVER,"readBlock: address: %x, buffer: %p\n", address, buffer);

  uint32_t response;
  mmc_send_cmd(17,address,&response,1);
  uint32_t* buffer32 = (uint32_t*) buffer;
//  uint8_t* buffer8 = (uint8_t*) buffer;
  uint32_t i = 0;
  while (i < sector_size_ / sizeof(uint32))
  {
    while (!(mmci->interrupt & (1 << 5)));
    buffer32[i++] = mmci->data;
  }
  return 0;
}

int32_t MMCDrive::readSector ( uint32_t start_sector, uint32_t num_sectors, void *buffer )
{
  debug(MMC_DRIVER,"readSector: start: %x, num: %x, buffer: %p\n", start_sector, num_sectors, buffer);
  for (uint32_t i = 0; i < num_sectors; ++i)
  {
    readBlock((start_sector + i) * sector_size_, (char*)buffer + i * sector_size_);
  }
  return 0;
}

int32_t MMCDrive::writeBlock ( uint32_t address, void *buffer)
{
  debug(MMC_DRIVER,"readBlock: address: %x, buffer: %p\n", address, buffer);
  uint32_t response;
  mmc_send_cmd(24, address, &response,1);
  uint32_t* buffer32 = (uint32_t*) buffer;
  //  uint8_t* buffer8 = (uint8_t*) buffer;
  uint32_t i = 0;
  while (i < sector_size_ / sizeof(uint32))
  {
    while (!(mmci->interrupt & (1 << 5)));
    mmci->data = buffer32[i++];
  }
  return 0;
}

int32_t MMCDrive::writeSector ( uint32_t start_sector, uint32_t num_sectors, void * buffer)
{
  debug(MMC_DRIVER,"writeSector: start: %x, num: %x, buffer: %p\n", start_sector, num_sectors, buffer);
  for (uint32_t i = 0; i < num_sectors; ++i)
  {
    writeBlock((start_sector + i) * sector_size_, (char*)buffer + i * sector_size_);
  }
  return 0;
}

uint32_t MMCDrive::getNumSectors()
{
  return num_sectors_;
}

uint32_t MMCDrive::getSectorSize()
{
  return sector_size_;
}

void MMCDrive::serviceIRQ()
{
}


MMCDeviceDriver::MMCDeviceDriver() :
    BasicDeviceDriver("MMC device driver")
{
}

MMCDeviceDriver& MMCDeviceDriver::instance()
{
    static MMCDeviceDriver instance_;
    return instance_;
}

void MMCDeviceDriver::doDeviceDetection()
{
    // Assume we have a MMC drive
    // Need to name this "idea" for compatibility with userspace disk detection
    // even though it has nothing to do with IDE
    constexpr const char* disk_name = "idea";
    MMCDrive* drv = new MMCDrive();
    bindDevice(*drv);

    auto *bdv = new BDVirtualDevice(drv, 0, drv->getNumSectors(), drv->getSectorSize(), disk_name, true);
    BDManager::instance().addVirtualDevice(bdv);

    detectMBRPartitions(bdv, drv, 0, drv->SPT, disk_name);
}
