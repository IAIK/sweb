#include "arch_serial.h"
#include "serial.h"

SerialPort::SerialPort ( ArchSerialInfo port_info )
{
  this->port_info_ = port_info;
};

SerialPort::~SerialPort ()
{
  
};

SerialPort::SRESULT SerialPort::setup_port( BAUD_RATE_E baud_rate, DATA_BITS_E data_bits, STOP_BITS_E stop_bits, PARITY_E parity )
{
  return SR_OK;
};

