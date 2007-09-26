/**
 * @file serial.h
 */

#ifndef SERIAL_H__
#define SERIAL_H__

#include "types.h"
#include "chardev.h"

#define MAX_PORTS  16

class ArchSerialInfo;

/**
 * @class SerialPort Class that describes Serial port
 *
 * The serial port class must not be instantiated ...
 * Use the SerialManager class and its serial_ports member for
 * serial port access ...
 */
class SerialPort : public CharacterDevice
{
  public:
    /**
     * @enum _br The baudrate for serial communication
     */
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

    /**
     * @enum _par The parity for serial communication
     */
    typedef enum _par
    {
        ODD_PARITY,
        EVEN_PARITY,
        NO_PARITY
    }
    PARITY_E;

    /**
     * @enum _db Number of data bits used in serial communication
     */
    typedef enum _db
    {
        DATA_7,
        DATA_8
    }
    DATA_BITS_E;

    /**
     * @enum _sb Number of stop bits used in serial communication
     */
    typedef enum _sb
    {
        STOP_ONE,
        STOP_TWO,
        STOP_ONEANDHALF
    }
    STOP_BITS_E;

    /**
     * @enum _sres Results returned by serial port functions
     */
    typedef enum _sres
    {
        SR_OK,
        SR_ERROR // TODO: add elements for common errors that can appear
    }
    SRESULT;

    SerialPort ( char*, ArchSerialInfo port_info );

    ~SerialPort ();

    /**
     * Opens a serial port for reading or writing
     * @param baud_rate Speed @see BAUD_RATE_E
     * @param data_bits Data bits @see DATA_BITS_E
     * @param stop_bits Stop bits @see STOP_BITS_E
     * @param parity Parity @see PARITY_E
     * @return Result @see SERIAL_ERROR_E
     */
    SRESULT setup_port ( BAUD_RATE_E baud_rate, DATA_BITS_E data_bits, STOP_BITS_E stop_bits, PARITY_E parity );

    /**
     * Writes size bytes to serial port
     * @param offset Not used with serial ports
     * @param size Number of bytes to be written
     * @param buffer The data to be written
     * @return Number of bytes actualy written or -1 in case of an error
     */
    virtual int32 writeData ( int32 offset, int32 size, const char*buffer );

//  /**
//  * Reads size bytes from the serial port
//  * @param offset Not used with serial ports
//  * @param size Number of bytes to be read
//  * @param buffer The buffer to store the data (must be preallocated)
//  * @return Number of bytes actualy read or -1 in case of an error
//  */
//  virtual int32 readData(int32 offset, int32 size, char *buffer);

    /**
     * Handles the irq of this serial port \sa get_info()
     */
    void irq_handler();

    /**
     * Returns the Architecture specific data for this serial port.
     * The basic task of any operating system is to hide this ugly data.
     * This function is mainly called from SerialManager and I do not think
     * that it is needed anywhere else. Perhaps it could be protected and
     * SerialManager declared as a friend class.
     * @return @see ArchSerialInfo
     */
    ArchSerialInfo get_info()
    {
      return port_info_;
    };

  private:
    void write_UART ( uint32 reg, uint8 what );
    uint8 read_UART ( uint32 reg );

    uint32 WriteLock;
    uint32 SerialLock;

  private:
    /**
     * Architecture specific data for serial ports
     */
    ArchSerialInfo port_info_;

};


/**
 * @class SerialManager Class that manages serial ports
 *
 * To access the serial ports use get_num_ports to find out
 * how many there are. Then access the wanted port with serial_ports[ port_number ].
 *
 * The following code will print out the names of the ports registered on system
 *
 * SerialManager *sm = SerialManager::getInstance();
 * uint32 num_ports = sm->get_num_ports();
 * for( uint32 i=0; i < num_ports; i++ )
 *   kprintf( "Port number : %d, Port name : %s", i , sm->serial_ports[i]->friendly_name );
 *
 * Or if you want to access the specific port
 *
 * SerialManager *sm = SerialManager::getInstance();
 * uint32 port_num = sm->get_port_number( (uint8 *) "COM1" ); // or "ttyS0"
 * SerialPort *com1 = sm->serial_ports[ port_num ];
 * // now do whatever you want with com1
 *
 * @see SerialPort
 */

class SerialManager
{
  public:
    static SerialManager *instance_;

    /**
     * returns the singleton Instance
     * @return the SerialManager instance
     */
    static SerialManager * getInstance()
    {
      if ( !instance_ )
        instance_ = new SerialManager();

      return instance_;
    };

    /**
     * Constructor
     */
    SerialManager();

    /**
     * Destructor
     */
    ~SerialManager();

    SerialPort * serial_ports[ MAX_PORTS ];

    /**
     * detects all serial ports
     * @param is_paging_set_up 1 if up 0 if not
     * @return number of ports found
     */
    uint32 do_detection ( uint32 is_paging_set_up );

    /**
     * returns the number of serial ports
     * @return the number of ports
     */
    uint32 get_num_ports();

    /**
     * returns the port number by name
     * @param friendly_name the name
     * @return the port number
     */
    uint32 get_port_number ( const uint8* friendly_name );

    /**
     * tells the serial port to handle the irq given by its number
     * @param irq_num the number of the irg to handle
     */
    void service_irq ( uint32 irq_num );

  private:
    uint32 num_ports;
};

#endif
