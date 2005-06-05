#include "ports.h"

#include "arch_serial.h"
#include "serial.h"

#include "ArchThreads.h"

//#include "kprintf.h"
//#include "debug_bochs.h"

#include "8259.h"


SerialPort::SerialPort ( ArchSerialInfo port_info ) : buffer_write_( 0 ), buffer_read_( 0 )
{
  this->port_info_ = port_info;

  WriteLock = 0;
  SerialLock = 0;
  
  setup_port( BR_9600, DATA_8, STOP_ONE, NO_PARITY );
};

SerialPort::~SerialPort ()
{
  
};

SerialPort::SRESULT SerialPort::setup_port( BAUD_RATE_E baud_rate, DATA_BITS_E data_bits, STOP_BITS_E stop_bits, PARITY_E parity )
{
  write_UART( SC::IER , 0x00); // turn off interupts 
  
  uint8 divisor = 0x0C;
  
  switch( baud_rate )
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
      
  write_UART( SC::LCR , 0x80);  // activate DL
  
  write_UART( 0 , divisor );    // DL low byte
  write_UART( SC::IER , 0x00);  // DL high byte  
  
  uint8 data_bit_reg = 0x03;
  
  switch( data_bits )
  {
    case DATA_8:
      data_bit_reg = 0x03;
      break;
    case DATA_7:
      data_bit_reg = 0x02;
      break;
  }
  
  uint8 par = 0x00;
  
  switch( parity )
  {
    case NO_PARITY:
      par = 0x00;
      break;
    case EVEN_PARITY:
      par = 0x18;
      break;
    case ODD_PARITY:
      par = 0x08;
      break;
  }
      
  uint8 stopb = 0x00;
  
  switch( stop_bits )
  {
    case STOP_ONE:
      stopb = 0x00;
      break;
    case STOP_TWO:
    case STOP_ONEANDHALF:
      stopb = 0x04;
      break;
  }
  
  write_UART( SC::LCR , data_bit_reg | par | stopb );  // deact DL and set params
  
  write_UART( SC::FCR , 0xC7);
  write_UART( SC::MCR , 0x0B);  
  
  enableIRQ( this->port_info_.irq_num ); 
  write_UART( SC::IER , 0x0F);  
  
  return SR_OK;
};

SerialPort::SRESULT SerialPort::write( uint8 *buffer, uint32 num_bytes, uint32& bytes_written )
{
  uint32 jiffies = 0;
  
  while( ArchThreads::testSetLock( SerialLock ,1 ) && jiffies++ < 50000 );
    
  if( jiffies == 50000 )
  {
    SerialLock = 0;
    WriteLock = 0;
    return SR_ERROR;
  }
  
  WriteLock = bytes_written = 0;
  
  while( num_bytes -- )
  {    
    jiffies = 0;
         
    while( !(read_UART( SC::LSR ) & 0x40) && jiffies++ < 50000 );
    
    if( jiffies == 50000 ) // TIMEOUT
    {
      SerialLock = 0;
      WriteLock = 0;
      return SR_ERROR;
    }    
    
    write_UART( 0, *(buffer++) );
    bytes_written++;
  }
    
  SerialLock = 0;
  return SR_OK;
};

SerialPort::SRESULT SerialPort::read( uint8 *buffer, uint32 num_bytes, uint32& bytes_read )
{
  if( buffer_write_ <= buffer_read_ )
  {
    bytes_read = 0;
    return SR_ERROR;
  }
  
  if( ( buffer_write_ - buffer_read_ ) < num_bytes )
    num_bytes = buffer_write_ - buffer_read_;
  
  uint8 * pb = (portbuffer_ + (buffer_read_ % 1024));
  buffer_read_ += num_bytes;
  bytes_read = num_bytes;
    
  while (num_bytes--)
  {
      *buffer++ = *pb++;
  }
  
  return SR_OK;
};

void SerialPort::irq_handler()
{
  do
  {
    uint8 int_id_reg = read_UART( SC::IIR );
    
    if( int_id_reg & 0x01 )
      return; // it is not my IRQ or IRQ is handled
  
    uint8 int_id = (int_id_reg & 0x06) >> 1;
    
    switch( int_id )
    {
    case 0: // Modem status changed
      break;
    case 1: // Output buffer is empty
      WriteLock = 0;
      break;
    case 2: // Data is available
      int_id = read_UART( 0 );
      portbuffer_[ buffer_write_ % 2048 ] = int_id;
      buffer_write_++;
      break;
    case 3: // Line status changed
      break;
    default: // This will never be executed
      break;
    }
  }
  while(1);
  
  return;
}

void SerialPort::write_UART( uint32 reg, uint8 what )
{
   outportb( this->port_info_.base_port + reg, what );
};

uint8 SerialPort::read_UART( uint32 reg )
{
  return inportb( this->port_info_.base_port + reg );
};


