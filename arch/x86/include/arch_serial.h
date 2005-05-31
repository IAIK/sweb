#ifndef _ARCH_SERIAL_H_
#define _ARCH_SERIAL_H_

#include "types.h"

/// \class ArchSerialInfo
/// \brief Class that contains architecture specific parameters for serial port communication

class ArchSerialInfo
{
public:
  uint16 base_port;  ///< The base IO port 
  uint8  uart_type;  ///< Type of the connected UART
  uint8  int_num;    ///< Interrupt number
};

/// \class SC
/// \brief Class that contains constants. Cleaner OO solution than #defining constants.

class SC
{
public:
  static uint32 IER;
  static uint32 IIR;
  static uint32 FCR;
  static uint32 LCR;
  static uint32 MCR;
  static uint32 LSR;
  static uint32 MSR;
};

#endif
