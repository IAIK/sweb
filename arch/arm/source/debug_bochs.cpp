/**
 * @file debug_bochs.cpp
 *
 */

#include "debug_bochs.h"
#include "ports.h"

#define SERIAL_BASE 0x16000000
#define SERIAL_FLAG_REGISTER 0x18
#define SERIAL_BUFFER_FULL (1 << 5)

void writeChar2Bochs( uint8 char2Write )
{
  /* Wait until the serial buffer is empty */
  while (*(volatile unsigned long*)(SERIAL_BASE + SERIAL_FLAG_REGISTER)
                                     & (SERIAL_BUFFER_FULL));
  /* Put our character, c, into the serial buffer */
  *(volatile unsigned long*)SERIAL_BASE = char2Write;
}

void writeLine2Bochs( const uint8 * line2Write )
{
  // TODO: It would be nice to have a sprintf function
  // so we can format the string
  const uint8 *currentChar;
  const uint8 *swebTag = ( uint8 * ) "[SWEB] ";

  for( currentChar = swebTag; (*currentChar != '\0'); currentChar++ )
    writeChar2Bochs( *currentChar );

  uint8 counter = 0; // the message is cut off at 250 chars 

  for( currentChar = line2Write; (*currentChar != '\0') && (counter++ < 250); currentChar++ )
    writeChar2Bochs( *currentChar );
}
