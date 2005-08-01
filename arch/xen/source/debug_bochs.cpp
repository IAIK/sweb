//----------------------------------------------------------------------
//  $Id: debug_bochs.cpp,v 1.1 2005/08/01 08:20:55 nightcreature Exp $
//----------------------------------------------------------------------
//
//  $Log: debug_bochs.cpp,v $
//
//----------------------------------------------------------------------


#include "debug_bochs.h"
#include "ports.h"

extern "C" int printf(const char *fmt, ...);

void writeChar2Bochs( uint8 char2Write )
{
  char tmp[2] = {char2Write,'\0'};
  printf(tmp);
  return;
}


void writeLine2Bochs( const uint8 * line2Write )
{
  printf((char*)line2Write);
  return;
}

