#include "ArchSerialInfo.h"
#include "ports.h"
#include "SerialManager.h"

#include "ArchInterrupts.h"
#include "ArchThreads.h"
#include "kprintf.h"
#include "8259.h"
#include "debug.h"

SerialPort::SerialPort(const char* name, const ArchSerialInfo& port_info) :
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

  SerialPort_FifoControlRegister fcr{};
  fcr.enable_fifos = 1;
  fcr.clear_receive_fifo = 1;
  fcr.clear_transmit_fifo = 1;
  fcr.enable_64_byte_fifo = 1;
  fcr.trigger_level = FIFO_TRIGGER_LEVEL::TRIGGER_16_OR_56_BYTES;

  write_UART(SerialPortRegister::FCR, fcr.u8);

  SerialPort_ModemControlRegister mcr{};
  mcr.data_terminal_ready = 1;
  mcr.request_to_send = 1;
  mcr.aux2 = 1;

  write_UART(SerialPortRegister::MCR, mcr.u8);

  SerialPort_InterruptEnableRegister ier{};
  ier.received_data_available_int_enable = 1;
  // ier.transmitter_holding_reg_empty_int_enable = 1; // Polling used instead
  ier.receiver_line_status_int_enable = 1;
  ier.modem_status_int_enable = 1;

  write_UART(SerialPortRegister::IER, ier.u8);

  return SR_OK;
}

size_t SerialPort::writeTransmitBuffer(const char* buffer, size_t size)
{
    size_t nwritten = 0;
    while (nwritten < size && eastl::bit_cast<SerialPort_LineStatusRegister>(
                                  read_UART(SerialPortRegister::LSR))
                                  .empty_transmitter_holding_reg)
    {
        char b = *(buffer + nwritten);
        debugAdvanced(A_SERIALPORT, "Write char to serial port: %c (%x)\n", b, b);
        write_UART(SerialPortRegister::THR, b);
        ++nwritten;
    }

    return nwritten;
}

int32 SerialPort::writeData(uint32 offset, uint32 num_bytes, const char* buffer)
{
  debug(A_SERIALPORT, "Write serial port, buffer: %p, size: %u\n", buffer, num_bytes);
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

  while(bytes_written < num_bytes)
  {
    jiffies = 0;

    size_t nwritten_chunk = writeTransmitBuffer(buffer + bytes_written, num_bytes - bytes_written);
    bytes_written += nwritten_chunk;

    if (bytes_written < num_bytes)
    {
        while (!eastl::bit_cast<SerialPort_LineStatusRegister>(
                    read_UART(SerialPortRegister::LSR))
                    .empty_transmitter_holding_reg &&
               jiffies++ < IO_TIMEOUT)
            ArchInterrupts::yieldIfIFSet();

        if (jiffies == IO_TIMEOUT)
        {
            SerialLock = 0;
            WriteLock = 0;
            return -1;
        }
    }
  }

  SerialLock = 0;
  return bytes_written;
}

void SerialPort::readReceiveBuffers()
{
    SerialPort_LineStatusRegister lsr{};
    lsr.u8 = read_UART(SerialPortRegister::LSR);
    while (lsr.data_ready)
    {
        auto b = read_UART(SerialPortRegister::RBR);
        debug(A_SERIALPORT, "Read char from serial port: %c (%x)\n", b, b);
        // TODO: FIFO uses a mutex internally -> deadlock when used in interrupt handler
        // (this has been broken since the beginning)
        in_buffer_.put(b);
        lsr.u8 = read_UART(SerialPortRegister::LSR);
    }
}


void SerialPort::irq_handler()
{
  debug(A_SERIALPORT, "irq_handler: Entered SerialPort IRQ handler\n");

  SerialPort_InterruptIdentificationRegister int_id_reg{};
  int_id_reg.u8 = read_UART(SerialPortRegister::IIR);
  debug(A_SERIALPORT, "irq_handler: IIR: %x\n", int_id_reg.u8);

  if (int_id_reg.int_pending)
  {
      debug(A_SERIALPORT, "Nothing (more) to do here\n");
      return; // it is not my IRQ or IRQ is handled
  }


  switch (int_id_reg.int_status)
  {
  case IIR_int::MODEM_STATUS: // Modem status changed
  {
      debug(A_SERIALPORT, "Modem status IRQ\n");
      SerialPort_ModemStatusRegister msr{};
      msr.u8 = read_UART(SerialPortRegister::MSR);
      debug(A_SERIALPORT, "Modem status: %x\n", msr.u8);
      break;
  }
  case IIR_int::TRANSMITTER_HOLDING_REG_EMPTY: // Output buffer is empty
  {
      debug(A_SERIALPORT, "Transmitter holding reg empty IRQ\n");
      WriteLock = 0;
      break;
  }
  case IIR_int::RECEIVED_DATA_AVAILABLE: // Data is available
  {
      debug(A_SERIALPORT, "Received data available IRQ\n");
      readReceiveBuffers();
      break;
  }
  case IIR_int::RECEIVER_LINE_STATUS: // Line status changed
  {
      debug(A_SERIALPORT, "Receiver line status IRQ\n");
      SerialPort_LineStatusRegister lsr{};
      lsr.u8 = read_UART(SerialPortRegister::LSR);
      debug(A_SERIALPORT, "Line status: %x\n", lsr.u8);

      if (lsr.overrun_error)
      {
          debugAlways(A_SERIALPORT, "Overrun error! Receive buffer is full, dropping incoming data\n");
      }
      if (lsr.parity_error)
      {
          debugAlways(A_SERIALPORT, "Parity error!\n");
      }
      if (lsr.framing_error)
      {
          debugAlways(A_SERIALPORT, "Framing error!\n");
      }
      if (lsr.break_interrupt)
      {
          debug(A_SERIALPORT, "Break interrupt\n");
      }
      if (lsr.empty_transmitter_holding_reg)
      {
          debug(A_SERIALPORT, "Empty transmitter holding register\n");
      }
      if (lsr.empty_data_holding_reg)
      {
          debug(A_SERIALPORT, "Empty data holding register\n");
      }
      if (lsr.fifo_receive_error)
      {
          debugAlways(A_SERIALPORT, "FIFO receive error! Clearing FIFO\n");
          SerialPort_FifoControlRegister fcr{};
          fcr.enable_fifos = 1;
          fcr.clear_receive_fifo = 1;
          fcr.enable_64_byte_fifo = 1;
          fcr.trigger_level = FIFO_TRIGGER_LEVEL::TRIGGER_16_OR_56_BYTES;

          write_UART(SerialPortRegister::FCR, fcr.u8);
      }
      break;
  }
  case IIR_int::TIMEOUT:
  {
      debug(A_SERIALPORT, "Timeout IRQ\n");
      readReceiveBuffers();
      break;
  }
  default: // This will never be executed
  {
      debug(A_SERIALPORT, "Unknown serial port IRQ\n");
      break;
  }
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
