#ifndef _SERIAL_H_
#define _SERIAL_H_

#include "types.h"
#include "chardev.h"

#define MAX_PORTS  16

/// \class Serial Port
/// \brief Class that describes Serial port
/// 
///  The serial port class must not be instantiated ... 
///  Use the SerialManager class and its serial_ports member for 
///  serial port access ...

class ArchSerialInfo;

class SerialPort : public CharacterDevice
{
public:

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

SerialPort ( char*, ArchSerialInfo port_info );

~SerialPort ();

/// \brief Opens a serial port for reading or writing
/// \param baud_rate Speed \sa BAUD_RATE_E
/// \param data_bits Data bits \sa DATA_BITS_E
/// \param parity Parity \sa PARITY_E
/// \param stop_bits Stop bits \sa STOP_BITS_E
/// \return Result \sa SERIAL_ERROR_E
SRESULT setup_port( BAUD_RATE_E baud_rate, DATA_BITS_E data_bits, STOP_BITS_E stop_bits, PARITY_E parity );

/// \brief Writes size bytes to serial port
/// \param buffer The data to be written
/// \param size Number of bytes to be written
/// \param offset Not used with serial ports
/// \return Number of bytes actualy written or -1 in case of an error
virtual int32 writeData(int32 offset, int32 size, const char*buffer);

/// \brief Reads size bytes from the serial port
/// \param buffer The buffer to store the data (must be preallocated)
/// \param size Number of bytes to be read
/// \param offset Not used with serial ports
/// \return Number of bytes actualy read or -1 in case of an error
//  virtual int32 readData(int32 offset, int32 size, char *buffer);
//  inherited from chardevice

/// \brief Handles the irq of this serial port \sa get_info()
void irq_handler();

/// \brief Returns the Architecture specific data for this serial port.
/// The basic task of any operating system is to hide this ugly data.
/// This function is mainly called from SerialManager and I do not think
/// that it is needed anywhere else. Perhaps it could be protected and 
/// SerialManager declared as a friend class.
/// \return \sa ArchSerialInfo
ArchSerialInfo get_info() 
{
  return port_info_;
};

private:
void write_UART( uint32 reg, uint8 what );
uint8 read_UART( uint32 reg );

uint32 WriteLock; 
uint32 SerialLock;

private:
ArchSerialInfo port_info_;  ///< Architecture specific data for serial ports

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
///  Or if you want to access the specific port 
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

uint32 do_detection( uint32 );
uint32 get_num_ports();

uint32 get_port_number( const uint8* friendly_name );

void service_irq( uint32 irq_num );

private:
uint32 num_ports;
};

#endif
