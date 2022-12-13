#include "ArchSerialInfo.h"
#include "ports.h"
#include "SerialManager.h"

#include "ArchInterrupts.h"
#include "ArchThreads.h"
#include "kprintf.h"
#include "8259.h"
#include "debug.h"


SerialPort::SerialPort(const char *name, const ArchSerialInfo& port_info) :
    CharacterDevice(name),
    IrqDomain(eastl::string("Serial Port ") + name)
{
  this->port_info_ = port_info;

  WriteLock = 0;
  SerialLock = 0;

  setup_port(BR_9600, DATA_8, STOP_ONE, NO_PARITY);

  auto irqnum = get_info().irq_num;
  IrqDomain::irq()
      .mapTo(ArchInterrupts::isaIrqDomain(), irqnum)
      .useHandler([this] { irq_handler(); });

  debug(DRIVER, "New serial port device, io port: %x, irq: %u\n", get_info().base_port, irqnum);
}

SerialPort::SRESULT SerialPort::setup_port(BAUD_RATE_E baud_rate, DATA_BITS_E data_bits, STOP_BITS_E stop_bits, PARITY_E parity)
{
  write_UART(SerialPortRegister::IER, 0x00); // turn off interupts

  uint8 divisor = 0x0C;

  switch(baud_rate)
  {
    case BR_14400:
    case BR_19200:
      divisor = 0x06;
      break;
    case BR_38400:
      divisor = 0x03;
      break;
    case BR_55600:
      divisor = 0x02;
      break;
    case BR_115200:
      divisor = 0x01;
      break;
    default:
    case BR_9600:
      divisor = 0x0C;
      break;
  }

  SerialPort_LineControlRegister lcr{};
  lcr.divisor_latch = 1;

  write_UART(SerialPortRegister::LCR, lcr.u8); // activate DL

  write_UART(SerialPortRegister::DLL, divisor); // DL low byte
  write_UART(SerialPortRegister::DLH, 0x00); // DL high byte

  lcr.divisor_latch = 0;

  switch(data_bits)
  {
    case DATA_8:
      lcr.word_length = LCR_word_length::BITS_8;
      break;
    case DATA_7:
      lcr.word_length = LCR_word_length::BITS_7;
      break;
  }


  switch(parity)
  {
    case NO_PARITY:
      lcr.parity = LCR_parity::NO_PARITY;
      break;
    case EVEN_PARITY:
      lcr.parity = LCR_parity::EVEN_PARITY;
      break;
    case ODD_PARITY:
      lcr.parity = LCR_parity::ODD_PARITY;
      break;
  }


  switch(stop_bits)
  {
    case STOP_ONE:
      lcr.stop_bits = LCR_stop_bits::ONE_STOP_BIT;
      break;
    case STOP_TWO:
    case STOP_ONEANDHALF:
      lcr.stop_bits = LCR_stop_bits::TWO_STOP_BITS;
      break;
  }

  write_UART(SerialPortRegister::LCR, lcr.u8); // deact DL and set params

  write_UART(SerialPortRegister::FCR, 0xC7);
  write_UART(SerialPortRegister::MCR, 0x0B);

  write_UART(SerialPortRegister::IER, 0x0F);

  return SR_OK;
}

int32 SerialPort::writeData(uint32 offset, uint32 num_bytes, const char*buffer)
{
  if(offset != 0)
    return -1;

  size_t jiffies = 0, bytes_written = 0;

  while(ArchThreads::testSetLock(SerialLock , (size_t)1) && jiffies++ < IO_TIMEOUT)
    ArchInterrupts::yieldIfIFSet();

  if(jiffies == IO_TIMEOUT)
  {
    WriteLock = 0;
    return -1;
  }

  WriteLock = 0;
  bytes_written = 0;

  while(num_bytes--)
  {
    jiffies = 0;

    while(!(read_UART(SerialPortRegister::LSR) & 0x40) && jiffies++ < IO_TIMEOUT)
      ArchInterrupts::yieldIfIFSet();

    if(jiffies == IO_TIMEOUT)
    {
      SerialLock = 0;
      WriteLock = 0;
      return -1;
    }

    write_UART(SerialPortRegister::THR, *(buffer++));
    bytes_written++;
  }

  SerialLock = 0;
  return bytes_written;
}

void SerialPort::irq_handler()
{
  debug(A_SERIALPORT, "irq_handler: Entered SerialPort IRQ handler");

  SerialPort_InterruptIdentificationRegister int_id_reg;
  int_id_reg.u8 = read_UART(SerialPortRegister::IIR);

  if (int_id_reg.int_pending)
      return;                    // it is not my IRQ or IRQ is handled


  switch (int_id_reg.int_status)
  {
  case IIR_int::MODEM_STATUS: // Modem status changed
      break;
  case IIR_int::TRANSMITTER_HOLDING_REG_EMPTY: // Output buffer is empty
  {
      WriteLock = 0;
      break;
  }
  case IIR_int::RECEIVED_DATA_AVAILABLE: // Data is available
  {
      auto b = read_UART(SerialPortRegister::RBR);
      in_buffer_.put(b);
      break;
  }
  case IIR_int::RECEIVER_LINE_STATUS: // Line status changed
      break;
  case IIR_int::TIMEOUT:
      break;
  default: // This will never be executed
      break;
  }
}

void SerialPort::write_UART(uint16_t base_port, SerialPortRegister reg, uint8 what)
{
    outportb(base_port + static_cast<uint32_t>(reg), what);
}

uint8 SerialPort::read_UART(uint16_t base_port, SerialPortRegister reg)
{
    return inportb(base_port + static_cast<uint32_t>(reg));
}

void SerialPort::write_UART(SerialPortRegister reg, uint8 what)
{
    write_UART(port_info_.base_port, reg, what);
}

uint8 SerialPort::read_UART(SerialPortRegister reg)
{
    return read_UART(port_info_.base_port, reg);
}
