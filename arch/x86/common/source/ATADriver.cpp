#include "ATADriver.h"

#include "BDManager.h"
#include "BDRequest.h"
#include "ArchInterrupts.h"
#include "8259.h"
#include "APIC.h"
#include "IDEDriver.h"

#include "Scheduler.h"
#include "kprintf.h"

#include "Thread.h"
#include "offsets.h"

#define TIMEOUT_WARNING() do { kprintfd("%s:%d: timeout. THIS MIGHT CAUSE SERIOUS TROUBLE!\n", __PRETTY_FUNCTION__, __LINE__); } while (0)

#define TIMEOUT_CHECK(CONDITION, BODY)            \
    jiffies = 0;                                  \
    while ((CONDITION) && jiffies++ < IO_TIMEOUT) \
        ArchInterrupts::yieldIfIFSet();           \
    if (jiffies >= IO_TIMEOUT)                    \
    {                                             \
        BODY;                                     \
    }

ATADriver::ATADriver(IDEControllerChannel& ide_controller, uint16 drive_num, eastl::span<uint16_t, 256> identify) :
    BDDriver(ide_controller.isaIrqNumber()),
    Device(eastl::string("ATA disk ") + eastl::to_string(drive_num)),
    controller(ide_controller),
    drive_num(drive_num),
    jiffies(0),
    irq_domain(eastl::string("ATA disk ") + eastl::to_string(drive_num)),
    lock_("ATADriver::lock_")
{
  irq_domain.irq()
      .mapTo(controller)
      .useHandler([]() { BDManager::instance().probeIRQ = false; });

  printIdentifyInfo(identify);

  lba = identify[49] & (1 << 9);
  numsec = *(uint32_t*)&identify[60];

  HPC = identify[3];
  SPT = identify[6];
  uint32 CYLS = identify[1];

  if (!numsec)
      numsec = CYLS * HPC * SPT;

  uint32_t logical_sector_size = *(uint32_t*)&identify[117];
  sector_word_size = logical_sector_size ? logical_sector_size : 256;

  debug(ATA_DRIVER, "Using LBA: %u, # sectors: %u, sector size: %zu\n", lba, numsec, sector_word_size*sizeof(uint16_t));

  debug(ATA_DRIVER, "Enabling interrupts for ATA IRQ check\n");

  {
      WithInterrupts intr(true);
      ArchInterrupts::enableIRQ(irq_domain.irq());
      testIRQ();
  }

  debug(ATA_DRIVER, "ctor: Using ATA mode: %d !!\n", (int)mode);

  // Use real handler from now on
  irq_domain.irq().useHandler([this]() { serviceIRQ(); });

  debug(ATA_DRIVER, "ctor: Drive created !!\n");
}

void ATADriver::testIRQ()
{
  mode = BD_ATA_MODE::BD_PIO;

  BDManager::instance().probeIRQ = true;
  controller.selectDrive(drive_num);
  readSector(0, 1, nullptr);

  debug(ATA_DRIVER, "Waiting for ATA IRQ\n");
  TIMEOUT_CHECK(BDManager::instance().probeIRQ,mode = BD_ATA_MODE::BD_PIO_NO_IRQ;);
}

int32 ATADriver::rawReadSector(uint32 start_sector, uint32 num_sectors, void *buffer)
{
  BD_ATA_MODE old_mode = mode;
  mode = BD_ATA_MODE::BD_PIO_NO_IRQ;
  uint32 result = readSector(start_sector, num_sectors, buffer);
  mode = old_mode;

  return result;
}

int32 ATADriver::selectSector(uint32 start_sector, uint32 num_sectors)
{
    if (!controller.waitNotBusy())
        return -1;

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


  IDEControllerChannel::IoRegister::DriveHead dh =
      controller.io_regs[IDEControllerChannel::IoRegister::DRIVE_HEAD].read();
  dh.drive_num = drive_num;
  dh.use_lba = lba;
  dh.always_set_0 = 1;
  dh.always_set_1 = 1;

  if (lba)
  {
      dh.lba_24_27 = (start_sector >> 24) & 0xF;
  }
  else
  {
      dh.chs_head_0_3 = head;
  }

  controller.io_regs[IDEControllerChannel::IoRegister::DRIVE_HEAD].write(dh);

  controller.io_regs[IDEControllerChannel::IoRegister::SECTOR_COUNT].write(num_sectors);

  if (lba)
  {
      controller.io_regs[IDEControllerChannel::IoRegister::LBA_LOW].write(start_sector &
                                                                          0xFF);
      controller.io_regs[IDEControllerChannel::IoRegister::LBA_MID].write(
          (start_sector >> 8) & 0xFF);
      controller.io_regs[IDEControllerChannel::IoRegister::LBA_HIGH].write(
          (start_sector >> 16) & 0xFF);
  }
  else
  {
      controller.io_regs[IDEControllerChannel::IoRegister::SECTOR_NUMBER].write(sect);
      controller.io_regs[IDEControllerChannel::IoRegister::CYLINDER_LOW].write(lo);
      controller.io_regs[IDEControllerChannel::IoRegister::CYLINDER_HIGH].write(high);
  }

  if (!controller.waitDriveReady())
      return -1;

  // /* Wait for drive to set DRDY */
  // TIMEOUT_CHECK(
  //     !controller.control_regs[IDEControllerChannel::ControlRegister::ALT_STATUS].read().ready,
  //     TIMEOUT_WARNING();
  //     return -1;);

  return 0;
}

void ATADriver::pioReadData(eastl::span<uint16_t> buffer)
{
    asm volatile("cld\n" // Ensure correct direction (low to high)
                 "rep insw\n" ::
        "D"(buffer.data()), // RDI
        "c"(buffer.size()), // RCX
        "d"(controller.io_regs[IDEControllerChannel::IoRegister::DATA].port)); // RDX
}

void ATADriver::pioWriteData(eastl::span<uint16_t> buffer)
{
    // Don't use rep outsw here because of required delay after port write
    for (uint16_t& word : buffer)
    {
        controller.io_regs[IDEControllerChannel::IoRegister::DATA].write(word);
    }
}

int32 ATADriver::readSector(uint32 start_sector, uint32 num_sectors, void *buffer)
{
  debugAdvanced(ATA_DRIVER, "readSector %x, num: %x into buffer %p\n", start_sector, num_sectors, buffer);

  assert(buffer || (start_sector == 0 && num_sectors == 1));
  if (selectSector(start_sector, num_sectors) != 0)
    return -1;

  if (!controller.waitNotBusy())
  {
      return -1;
  }

  controller.sendCommand(COMMAND::PIO::READ_SECTORS);

  if (mode != BD_ATA_MODE::BD_PIO_NO_IRQ)
      return 0;

  if (!controller.waitDataReady())
  {
      return -1;
  }

  if (buffer)
  {
    pioReadData({static_cast<uint16_t*>(buffer), sector_word_size*num_sectors});
  }

  if (!controller.waitNotBusy())
      return -1;

  return 0;
}

int32 ATADriver::writeSector(uint32 start_sector, uint32 num_sectors, void * buffer)
{
  assert(buffer);
  if (selectSector(start_sector, num_sectors) != 0)
    return -1;

  controller.sendCommand(COMMAND::PIO::WRITE_SECTORS);

  if (!controller.waitDataReady())
      return -1;

  uint32 count = (sector_word_size*num_sectors);
  if( mode != BD_ATA_MODE::BD_PIO_NO_IRQ )
    count = sector_word_size;

  pioWriteData({static_cast<uint16_t*>(buffer), count});

  if (!controller.waitNotBusy())
      return -1;

  controller.sendCommand(COMMAND::OTHER::FLUSH_CACHE);

  if (!controller.waitNotBusy())
      return -1;

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
      res = readSector(br->getStartBlock(), br->getNumBlocks(), br->getBuffer());
      break;
  case BDRequest::BD_CMD::BD_WRITE:
      res = writeSector(br->getStartBlock(), br->getNumBlocks(), br->getBuffer());
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
    {
        ArchInterrupts::enableInterrupts();
    }

    jiffies = 0;

    while (br->getStatus() == BDRequest::BD_RESULT::BD_QUEUED && jiffies++ < IO_TIMEOUT*10);

    if (jiffies >= IO_TIMEOUT*10)
    {
        [[maybe_unused]] auto data_ready = controller.waitDataReady();

        TIMEOUT_WARNING();
        auto status =
            controller.control_regs[IDEControllerChannel::ControlRegister::ALT_STATUS]
                .read();
        auto error = controller.io_regs[IDEControllerChannel::IoRegister::ERROR].read();

        debug(ATA_DRIVER, "addRequest: Device status: %x, error: %x\n", status.u8,
              error.u8);
    }

    if (br->getStatus() == BDRequest::BD_RESULT::BD_QUEUED)
    {
      debug(ATA_DRIVER, "addRequest: Request is still pending, going to sleep\n");
      ArchInterrupts::disableInterrupts();
      if (br->getStatus() == BDRequest::BD_RESULT::BD_QUEUED)
      {
        currentThread->setState(Thread::Sleeping);
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

void ATADriver::serviceIRQ()
{
    // Need to read status register to acknowledge and unblock interrupts
    auto status = controller.io_regs[IDEControllerChannel::IoRegister::STATUS].read();
    auto error = controller.io_regs[IDEControllerChannel::IoRegister::ERROR].read();
    auto lba_l = controller.io_regs[IDEControllerChannel::IoRegister::LBA_LOW].read();
    auto lba_m = controller.io_regs[IDEControllerChannel::IoRegister::LBA_MID].read();
    auto lba_h = controller.io_regs[IDEControllerChannel::IoRegister::LBA_HIGH].read();

    debugAdvanced(ATA_DRIVER, "serviceIRQ: Device status: %x, error: %x, lba_l: %x, lba_m: %x, lba_h: %x\n", status.u8, error.u8, lba_l, lba_m, lba_h);

    if (mode == BD_ATA_MODE::BD_PIO_NO_IRQ)
        return;

    BDRequest* br = request_list_.peekBack();

    if (br == nullptr)
    {
        debug(ATA_DRIVER,
              "serviceIRQ: IRQ without request! Device status: %x, error: %x, lba_l: %x, "
              "lba_m: %x, lba_h: %x\n",
              status.u8, error.u8, lba_l, lba_m, lba_h);
        if (status.error || status.drive_fault_error)
        {
            debugAlways(ATA_DRIVER, "serviceIRQ: Device error, reset device! Status: %x, Error: %x\n", status.u8, error.u8);
            controller.reset();
        }
        return; // not my interrupt
  }

  Thread* requesting_thread = br->getThread();

  debug(ATA_DRIVER, "serviceIRQ: Found active request!!\n");
  assert(br);

  uint16* word_buff = (uint16*) br->getBuffer();
  uint32 blocks_done = br->getBlocksDone();
  uint16_t* buf_ptr = word_buff + blocks_done*sector_word_size;

  // TODO: target thread may not yet be sleeping (this irq handler and section with disabled interrupts in addRequest may run simultaneously if running on other cpu core)

  if (br->getCmd() == BDRequest::BD_CMD::BD_READ)
  {
      debug(ATA_DRIVER, "serviceIRQ: Handling read request\n");
    // This IRQ handler may be run in the context of any arbitrary thread, so we must not attempt to access userspace
    assert((size_t)word_buff >= USER_BREAK);
    if (!controller.waitDataReady())
    {
      assert(request_list_.popBack() == br);
      br->setStatus(BDRequest::BD_RESULT::BD_ERROR); // br may be deallocated as soon as status is set to error
      debug(ATA_DRIVER, "serviceIRQ: Error while waiting for controller\n");
      if (requesting_thread)
          requesting_thread->setState(Thread::Running);
      return;
    }

    pioReadData({buf_ptr, sector_word_size});

    blocks_done++;
    br->setBlocksDone(blocks_done);

    if (blocks_done == br->getNumBlocks())
    {
      assert(request_list_.popBack() == br);
      br->setStatus(BDRequest::BD_RESULT::BD_DONE); // br may be deallocated as soon as status is set to done
      debug(ATA_DRIVER, "serviceIRQ: Read finished\n");
      if (requesting_thread)
          requesting_thread->setState(Thread::Running);
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
          requesting_thread->setState(Thread::Running);
    }
    else
    {
      // This IRQ handler may be run in the context of any arbitrary thread, so we must not attempt to access userspace
      assert((size_t)word_buff >= USER_BREAK);
      if (!controller.waitDataReady())
      {
        assert(request_list_.popBack() == br);
        br->setStatus(BDRequest::BD_RESULT::BD_ERROR); // br may be deallocated as soon as status is set to error
        if (requesting_thread)
            requesting_thread->setState(Thread::Running);
        return;
      }

      pioWriteData({buf_ptr, sector_word_size});

      br->setBlocksDone(blocks_done);
    }
  }
  else
  {
    blocks_done = br->getNumBlocks();
    assert(request_list_.popBack() == br);
    br->setStatus(BDRequest::BD_RESULT::BD_ERROR); // br may be deallocated as soon as status is set to error
    if (requesting_thread)
        requesting_thread->setState(Thread::Running);
  }

  debug(ATA_DRIVER, "serviceIRQ: Request handled!!\n");
}


void ATADriver::printIdentifyInfo(eastl::span<uint16_t, 256> id)
{
    uint16_t serialnum[11]{};
    uint16_t firmware_rev[5]{};
    uint16_t model_number[21]{};
    uint32_t num_sectors_28bit = 0;
    uint64_t num_logical_sectors = 0;
    // default 256 words / 512 bytes if 0
    uint32_t logical_sector_size = 0;
    memcpy(serialnum, &id[10], 20);
    memcpy(firmware_rev, &id[23], 8);
    memcpy(model_number, &id[27], 40);
    memcpy(&num_sectors_28bit, &id[60], 4);
    memcpy(&num_logical_sectors, &id[100], 8);
    memcpy(&logical_sector_size, &id[117], 4);
    for (auto& x : serialnum)
        x = __builtin_bswap16(x);
    for (auto& x : firmware_rev)
        x = __builtin_bswap16(x);
    for (auto& x : model_number)
        x = __builtin_bswap16(x);

    debug(ATA_DRIVER, "Device serial number: %s\n", (char*)serialnum);
    debug(ATA_DRIVER, "Firmware revision: %s\n", (char*)firmware_rev);
    debug(ATA_DRIVER, "Model number: %s\n", (char*)model_number);
    debug(ATA_DRIVER, "Cylinder: %u, Head: %u, Sector: %u, C*H*S = %u\n", id[1],
          id[3], id[6], id[1] * id[3] * id[6]);
    debug(ATA_DRIVER, "DRQ data block size: %u\n", (id[49] & 0xFF));
    debug(ATA_DRIVER, "Capabilities: LBA %u, DMA %u, IORDY %u, IORDY DISABLE: %u\n",
          !!(id[49] & (1 << 9)), !!(id[49] & (1 << 8)),
          !!(id[49] & (1 << 11)), !!(id[49] & (1 << 10)));
    debug(ATA_DRIVER, "#Sectors (28 bit): %u\n", *(uint32_t*)&num_sectors_28bit);
    debug(ATA_DRIVER,
          "Read/write multiple supported: %u, optimal multiple sector num: %u\n",
          !!(id[59] & (1 << 8)), (id[59] & 0xFF));
    debug(ATA_DRIVER, "Multiword DMA support: mode 0: %u, mode 1: %u, mode 2: %u\n",
          !!(id[63] & (1 << 0)), !!(id[63] & (1 << 1)),
          !!(id[63] & (1 << 2)));
    debug(ATA_DRIVER, "PIO support: max original: %u, PIO3: %u, PIO4: %u\n", (id[51] >> 8), !!(id[64] & (1 << 0)),
          !!(id[64] & (1 << 1)));
    debug(ATA_DRIVER, "Extended num of user addressable sectors supported: %u\n",
          !!(id[69] & (1 << 3)));
    debug(ATA_DRIVER, "Max queue depth: %u\n", (id[75] & 0b11111));
    debug(ATA_DRIVER,
          "Major version: ATA/ATAPI5: %u, ATA/ATAPI6: %u, ATA/ATAPI7: %u, "
          "ATA8-ACS: %u, ACS-2: %u, ACS-3: %u\n",
          !!(id[80] & (1 << 5)), !!(id[80] & (1 << 6)),
          !!(id[80] & (1 << 7)), !!(id[80] & (1 << 8)),
          !!(id[80] & (1 << 9)), !!(id[80] & (1 << 10)));
    debug(ATA_DRIVER,
          "Command/feature support: SMART: %u, Security: %u, Power "
          "management: %u, PACKET: %u, volatile write cache: %u, read "
          "look-ahead: %u, DEVICE RESET: %u, WRITE BUFFER: %u, READ "
          "BUFFER: %u, NOP: %u\n",
          !!(id[82] & (1 << 0)), !!(id[82] & (1 << 1)),
          !!(id[82] & (1 << 3)), !!(id[82] & (1 << 4)),
          !!(id[82] & (1 << 5)), !!(id[82] & (1 << 6)),
          !!(id[82] & (1 << 9)), !!(id[82] & (1 << 12)),
          !!(id[82] & (1 << 13)), !!(id[82] & (1 << 14)));
    debug(ATA_DRIVER, "48-bit address support: %u\n", !!(id[83] & (1 << 10)));
    debug(ATA_DRIVER,
          "UDMA support: m0: %u, m1: %u, m2: %u, m3: %u, m4: %u, m5: %u, "
          "m6: %u\n",
          !!(id[88] & (1 << 0)), !!(id[88] & (1 << 1)),
          !!(id[88] & (1 << 2)), !!(id[88] & (1 << 3)),
          !!(id[88] & (1 << 4)), !!(id[88] & (1 << 5)),
          !!(id[88] & (1 << 6)));
    debug(ATA_DRIVER,
          "UDMA active: m0: %u, m1: %u, m2: %u, m3: %u, m4: %u, m5: %u, "
          "m6: %u\n",
          !!(id[88] & (1 << 8)), !!(id[88] & (1 << 9)),
          !!(id[88] & (1 << 10)), !!(id[88] & (1 << 11)),
          !!(id[88] & (1 << 12)), !!(id[88] & (1 << 13)),
          !!(id[88] & (1 << 14)));
    debug(ATA_DRIVER, "#Sectors: %lu\n", *(uint64_t*)&num_logical_sectors);
    if (!(id[106] & (1 << 15)) && (id[106] & (1 << 14)))
    {
        debug(ATA_DRIVER, "Physical sector size: 2^%u*logical sectors\n",
              (id[106] & 0xF));
        debug(ATA_DRIVER,
              "Logical sector size > 512: %u, multiple logical sectors per "
              "phys sectors: %u\n",
              !!(id[106] & (1 << 12)), !!(id[106] & (1 << 13)));
        debug(ATA_DRIVER, "Logical sector size (words): %u\n",
              *(uint32_t*)&logical_sector_size);
    }

    debug(ATA_DRIVER, "Transport type: %u (0 = parallel, 1 = serial)\n",
          (id[222] & (0xF << 12)));
    if ((id[222] & (0xF << 12)) >> 12 == 0)
    {
        debug(ATA_DRIVER, "Transport version: ATA8-APT: %u, ATA/ATAPI-7: %u\n",
              !!(id[222] & (1 << 0)), !!(id[222] & (1 << 1)));
    }
    if ((id[222] & (0xF << 12)) >> 12 == 1)
    {
        debug(ATA_DRIVER,
              "Transport version: ATA8-AST: %u, SATA 1.0a: %u, SATA II "
              "Extensions: %u, SATA 2.5: %u, SATA 2.6: %u, SATA 3.0: %u, "
              "SATA: 3.1: %u\n",
              !!(id[222] & (1 << 0)), !!(id[222] & (1 << 1)),
              !!(id[222] & (1 << 2)), !!(id[222] & (1 << 3)),
              !!(id[222] & (1 << 4)), !!(id[222] & (1 << 5)),
              !!(id[222] & (1 << 6)));
    }
    debug(ATA_DRIVER, "Integrity word: %x, validity indicator: %x\n",
          (id[255] & (0xFF << 8)), (id[255] & 0xFF));
}
