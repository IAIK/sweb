#ifndef _ARCH_SERIAL_H_
#define _ARCH_SERIAL_H_

#include "types.h"


/**
 * @class ArchSerialInfo
 *
 * Class that contains architecture specific parameters for serial port communication
 *
 */
class ArchSerialInfo
{
public:
  uint16 base_port;  /// The base IO port
  uint8  uart_type;  /// Type of the connected UART
  uint8  irq_num;    /// IRQ number
};

/**
 * @class SC
 *
 * Class that contains constants. Cleaner OO solution than #defining constants
 *
 */
class SC
{
public:

  static uint32 IER;  /// UART Interrupt Enable Register
  static uint32 IIR;  /// UART Interrupt Indetification Register
  static uint32 FCR;  /// UART FIFO Control Register
  static uint32 LCR;  /// UART Line Control Register
  static uint32 MCR;  /// UART Modem Control Register
  static uint32 LSR;  /// UART Line Status Register
  static uint32 MSR;  /// UART Modem Status Register
  
  static uint32 UART_16650A;  /// Most common UART with FIFO buffers
  static uint32 UART_16650;   /// Old type UART with FIFO buffers that do not work
  static uint32 UART_OLD;     /// Old type UART without FIFO buffers
  
  static uint32 MAX_ARCH_PORTS;  /// Maximum serial ports registered in BIOS
};

#endif
