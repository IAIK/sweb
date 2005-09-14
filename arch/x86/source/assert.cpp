
//
// CVS Log Info for $RCSfile: assert.cpp,v $
//
// $Id: assert.cpp,v 1.5 2005/09/14 13:21:20 davrieb Exp $
// $Log: assert.cpp,v $
// Revision 1.4  2005/09/13 16:25:21  davrieb
// fix typo in assertion message
//
// Revision 1.3  2005/07/26 17:45:25  nomenquis
// foobar
//
// Revision 1.2  2005/07/07 14:56:53  davrieb
// fix assert to actually chaeck the condition
//
// Revision 1.1  2005/05/31 20:25:28  btittelbach
// moved assert to where it belongs (arch) and created nicer version
//
// Revision 1.1  2005/05/10 15:27:55  davrieb
// move assert to util/assert.h
//
//

#include "assert.h"
#include "kprintf.h"
#include "panic.h"


void pre_new_sweb_assert(uint32 condition, uint32 line, char* file)
{

    
  char *error_string = "KERNEL PANIC: Assertion Failed in File:  on Line: ";
  if (!condition)
  {
    uint8 * fb = (uint8*)0xC00B8000;
    uint32 s=0;
    uint32 i=0;
    for (s=0; s<40; ++s)
    {
      fb[i++] = error_string[s];
      fb[i++] = 0x9f;      
    }

    while (file && *file)
    {
      fb[i++] = *file++;
      fb[i++] = 0x9f;
    }

    for (s=40; s<50; ++s)
    {
      fb[i++] = error_string[s];
      fb[i++] = 0x9f;      
    }

    i+=6;
    while (line>0)
    {
      fb[i++] = (uint8) ( 0x30 + (line%10) );
      fb[i] = 0x9f; 
      i-=2;
      i--;
      line /= 10;
    }

    for (;;);
  }
}  


void sweb_assert(const char *condition, uint32 line, const char* file)
{
  kprintf_nosleep("KERNEL PANIC: Assertion %s failed in File %s on Line %d\n",condition, file, line);
  kpanict((uint8*) "Halting System\n");
}
