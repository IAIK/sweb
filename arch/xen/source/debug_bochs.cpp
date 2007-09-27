//----------------------------------------------------------------------
//  $Id: debug_bochs.cpp,v 1.2 2005/09/28 16:35:43 nightcreature Exp $
//----------------------------------------------------------------------
//
//  $Log: debug_bochs.cpp,v $
//  Revision 1.1  2005/08/01 08:20:55  nightcreature
//  needed to satisfy linker...should be removed
//
//
//----------------------------------------------------------------------


#include "debug_bochs.h"
#include "ports.h"

extern "C" int xenprintf(const char *fmt, ...);

void writeChar2Bochs( uint8 char2Write )
{
  char tmp[2] = {char2Write,'\0'};
  xenprintf(tmp);
  return;
}


void writeLine2Bochs( const uint8 * line2Write )
{
  xenprintf((char*)line2Write);
  return;
}

