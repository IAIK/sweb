#include "ArchSerialInfo.h"
#include "SerialManager.h"
#include "kstring.h"
#include "ArchInterrupts.h"

#include "debug_bochs.h"
#include "kprintf.h"
#include "8259.h"
#include "APIC.h"


SerialManager::SerialManager() :
    base_type("Serial Port Driver"),
    num_ports( 0 )
{
    debug(A_SERIALPORT, "Init serial port driver\n");
}

uint32 SerialManager::get_num_ports() const
{
  return num_ports;
}

void SerialManager::doDeviceDetection()
{
    do_detection(false);
}

UART_TYPE identifyUartDevice(uint16_t base_port)
{
    SerialPort::write_UART(base_port, SerialPortRegister::FCR, 0xE7);

    SerialPort_InterruptIdentificationRegister iir{};
    iir.u8 = SerialPort::read_UART(base_port, SerialPortRegister::IIR);

    switch (iir.fifo_info)
    {
    case IIR_fifo_info::NO_FIFO:
        debug(A_SERIALPORT, "Serial port has no fifo -> 8250\n");
        return UART_TYPE::UART_8250;
        break;
    case IIR_fifo_info::FIFO_NON_FUNCTIONAL:
        debug(A_SERIALPORT, "Serial port has non functional fifo -> 16550\n");
        return UART_TYPE::UART_16650;
        break;
    case IIR_fifo_info::FIFO_ENABLED:
        if (iir.fifo_64b_enabled)
        {
            debug(A_SERIALPORT, "Serial port has functional 64 byte fifo -> 16750\n");
            return UART_TYPE::UART_16750;
        }
        else
        {
            debug(A_SERIALPORT, "Serial port has functional fifo -> 16550A\n");
            return UART_TYPE::UART_16650A;
        }
        break;
    };

    return UART_TYPE::UART_8250;
}

uint32 SerialManager::do_detection(uint32 is_paging_set_up)
{
  uint16* bios_sp_table;

  if (is_paging_set_up)
    bios_sp_table = (uint16 *) 0xC0000400;
  else
    bios_sp_table = (uint16 *) 0x00000400;
  uint32 i = 0;

  for (; i < SC::MAX_ARCH_PORTS; i++, bios_sp_table++)
  {
    if (*bios_sp_table != 0x00)
    {
      uint8 sp_name[] = { 's', 'p', (uint8) (num_ports + '1'), '\0' };
      ArchSerialInfo archInfo{};
      archInfo.base_port = *bios_sp_table;
      archInfo.uart_type = static_cast<uint8_t>(identifyUartDevice(archInfo.base_port));
      archInfo.irq_num = 4 - i%2;
      auto s_port = new SerialPort((char*)sp_name, archInfo);
      serial_ports[ num_ports ] = s_port;
      bindDevice(*s_port);
      ArchInterrupts::enableIRQ(s_port->irq());
      num_ports++;
    }
  }

  return num_ports;
}

void SerialManager::service_irq( uint32 irq_num )
{
  for(size_t i = 0;i < num_ports; i++)
    if( serial_ports[ i ]->get_info().irq_num == irq_num )
      serial_ports[ i ]->irq_handler();
}
