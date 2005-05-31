#ifndef _SERIAL_H_
#define _SERIAL_H_

#include "types.h"

#define MAX_PORTS  16

/// \class Serial Port
/// \brief Class that describes Serial port
/// 
///  The serial port class must not be instantiated ... 
///  Use the SerialManager class and its serial_ports member for 
///  serial port access ...

class ArchSerialInfo;

class SerialPort
{
public:
char friendly_name[6]; ///< The user friendly name of the port - "COM1" or "ttyS0"


/// \brief The baudrate for serial communication
typedef enum _br
{
  BR_9600,
  BR_14400,
  BR_19200,
  BR_38400,
  BR_55600,
  BR_115200
}
BAUD_RATE_E;

/// \brief The parity for serial communication
typedef enum _par
{
  ODD_PARITY,
  EVEN_PARITY,
  NO_PARITY
}
PARITY_E;

/// \brief Number of data bits used in serial communication
typedef enum _db
{
  DATA_7,
  DATA_8
}
DATA_BITS_E;

/// \brief Number of stop bits used in serial communication
typedef enum _sb
{
  STOP_ONE,
  STOP_TWO,
  STOP_ONEANDHALF
}
STOP_BITS_E;

/// \brief Results returned by serial port functions 
typedef enum _sres
{
  SR_OK,
  SR_ERROR // TODO: add elements for common errors that can appear
}
SRESULT;

SerialPort ( ArchSerialInfo port_info );

~SerialPort ();

/// \brief Opens a serial port for reading or writing
/// \param baud_rate Speed \sa BAUD_RATE_E
/// \param data_bits Data bits \sa DATA_BITS_E
/// \param parity Parity \sa PARITY_E
/// \param stop_bits Stop bits \sa STOP_BITS_E
/// \return Result \sa SERIAL_ERROR_E
SRESULT setup_port( BAUD_RATE_E baud_rate, DATA_BITS_E data_bits, STOP_BITS_E stop_bits, PARITY_E parity );

/// \brief Writes num_bytes bytes to serial port
/// \param buffer The data to be written
/// \param num_bytes Number of bytes to be written
/// \param bytes_written Number of bytes actually written
/// \return Result \sa SERIAL_ERROR_E
SRESULT write( uint8 *buffer, uint32 num_bytes, uint32& bytes_written );

/// \brief Reads num_bytes bytes from serial port
/// \param buffer Buffer for the data. Must be preallocated.
/// \param num_bytes Number of bytes to be read
/// \return Number of bytes actually read
/// \return Result \sa SERIAL_ERROR_E
SRESULT read( uint8 *buffer, uint32 num_bytes, uint32& bytes_read );

static void interrupt_handler();

private:
ArchSerialInfo port_info_;  ///< Architecture specific data for serial ports

uint8 portbuffer_[2048]; ///< Internal circular serial port buffer
uint16 buffer_write_; ///< Pointer to the write position
uint16 buffer_read_; ///< Pointer to the read position

};


/// \class SerialManager
/// \brief Class that manages serial ports
/// 
///  To access the serial ports use get_num_ports to find out 
///  how many there are. Then access the wanted port with serial_ports[ port_number ].
///  
///  The following code will print out the names of the ports registered on system
///   
///  SerialManager *sm = SerialManager::getInstance();
///  uint32 num_ports = sm->get_num_ports();
///  for( uint32 i=0; i < num_ports; i++ )
///     kprintf( "Port number : %d, Port name : %s", i , sm->serial_ports[i]->friendly_name );
///
///  Or if you wanr to access the specific port 
/// 
///  SerialManager *sm = SerialManager::getInstance();
///  uint32 port_num = sm->get_port_number( (uint8 *) "COM1" ); // or "ttyS0"
///  SerialPort *com1 = sm->serial_ports[ port_num ];
///  /* now do whatever you want with com1 */ 
///  
///  \sa SerialPort
///  

class SerialManager
{
public:
static SerialManager *instance_;

static SerialManager * SerialManager::getInstance() 
{ 
  if( !instance_ )
    instance_ = new SerialManager();
  
  return instance_;
};

SerialManager();
~SerialManager();

SerialPort * serial_ports[ MAX_PORTS ];

uint32 do_detection();
uint32 get_num_ports();

uint32 get_port_number( const uint8* friendly_name );

private:
uint32 num_ports;
};

#endif
