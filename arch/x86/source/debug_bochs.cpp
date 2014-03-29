/**
 * @file debug_bochs.cpp
 *
 */

#include "debug_bochs.h"
#include "ports.h"


void writeChar2Bochs( char char2Write )
{
  outportb( 0xE9, char2Write );
}

void writeLine2Bochs( const char * line2Write )
{
  // TODO: It would be nice to have a sprintf function
  // so we can format the string
  const char *currentChar;
  const char *swebTag = ( char * ) "[SWEB] ";

  for( currentChar = swebTag; (*currentChar != '\0'); currentChar++ )
    writeChar2Bochs( *currentChar );

  uint8 counter = 0; // the message is cut off at 250 chars 

  for( currentChar = line2Write; (*currentChar != '\0') && (counter++ < 250); currentChar++ )
    writeChar2Bochs( *currentChar );
}
