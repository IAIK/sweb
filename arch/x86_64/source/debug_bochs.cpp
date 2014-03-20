/**
 * @file debug_bochs.cpp
 *
 */

#include "debug_bochs.h"
#include "ports.h"
#include "offsets.h"


void writeChar2Bochs( uint8 char2Write )
{
  outportb( 0xE9, char2Write );
}

void writeLine2Bochs( const uint8 * line2Write )
{
  const uint8 *currentChar;

  uint8 counter = 0; // the message is cut off at 250 chars

  for( currentChar = line2Write; (*currentChar != '\0') && (counter++ < 250); currentChar++ )
    writeChar2Bochs( *currentChar );
}
