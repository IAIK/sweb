//----------------------------------------------------------------------
//   $Id: kprintf.cpp,v 1.17 2005/09/13 15:00:51 btittelbach Exp $
//----------------------------------------------------------------------
//   $Log: kprintf.cpp,v $
//   Revision 1.16  2005/09/07 00:33:52  btittelbach
//   +More Bugfixes
//   +Character Queue (FiFoDRBOSS) from irq with Synchronisation that actually works
//
//   Revision 1.15  2005/09/03 21:54:45  btittelbach
//   Syscall Testprogramm, actually works now ;-) ;-)
//   Test get autocompiled and autoincluded into kernel
//   one kprintfd bug fixed
//
//   Revision 1.14  2005/08/19 21:14:15  btittelbach
//   Debugging the Debugging Code
//
//   Revision 1.12  2005/08/04 20:47:43  btittelbach
//   Where is the Bug, maybe I will see something tomorrow that I didn't see today
//
//   Revision 1.11  2005/08/04 17:49:22  btittelbach
//   Improved (documented) arch_PageFaultHandler
//   Solution to Userspace Bug still missing....
//
//   Revision 1.10  2005/07/27 13:43:47  btittelbach
//   Interrupt On/Off Autodetection in Kprintf
//
//   Revision 1.9  2005/07/27 10:04:26  btittelbach
//   kprintf_nosleep and kprintfd_nosleep now works
//   Output happens in dedicated Thread using VERY EVIL Mutex Hack
//
//   Revision 1.8  2005/07/24 17:02:59  nomenquis
//   lots of changes for new console stuff
//
//   Revision 1.7  2005/07/05 16:15:48  btittelbach
//   kprintf.cpp
//
//   Revision 1.6  2005/06/05 07:59:35  nelles
//   The kprintf_debug or kprintfd are finished
//
//   Revision 1.5  2005/05/10 17:03:55  btittelbach
//   Kprintf Vorbereitung für Print auf Bochs Console
//   böse .o im source gelöscht
//
//   Revision 1.4  2005/04/26 21:38:43  btittelbach
//   Fifo/Pipe Template soweit das ohne Lock und CV zu implementiern ging
//   kprintf kennt jetzt auch chars
//
//   Revision 1.3  2005/04/24 20:31:33  btittelbach
//   kprintf now knows how to padd
//
//   Revision 1.2  2005/04/24 18:58:45  btittelbach
//   kprintf bugfix
//
//   Revision 1.1  2005/04/24 13:33:40  btittelbach
//   skeleton of a kprintf
//
//
//----------------------------------------------------------------------



#include "stdarg.h"
#include "kprintf.h"
#include "Console.h"
#include "Terminal.h"
#include "debug_bochs.h"
#include "ArchInterrupts.h"
#include "ipc/FiFoDRBOSS.h"

//it's more important to keep the messages that led to an error, instead of
//the ones following it, when the nosleep buffer gets full

void oh_writeCharWithSleep(char c)
{
  main_console->getActiveTerminal()->write(c);
}
void oh_writeStringWithSleep(char const* str)
{
  main_console->getActiveTerminal()->writeString(str);
}


void oh_writeCharDebugWithSleep(char c)
{
  //this blocks
  writeChar2Bochs((uint8) c);
}
void oh_writeStringDebugWithSleep(char const* str)
{
  //this blocks
  while (*str)
  {
    oh_writeCharDebugWithSleep(*str);
    str++;
  }
}


FiFoDRBOSS<char> *nosleep_fifo_;

void kprintf_nosleep_init()
{
  nosleep_fifo_ = new FiFoDRBOSS<char>(8192,2048,true);
}

void oh_writeCharNoSleep(char c)
{
  nosleep_fifo_->put(c);
}
void oh_writeStringNoSleep(char const* str)
{
  while (*str)
  {
    oh_writeCharNoSleep(*str);
    str++;
  }
}

void flushActiveConsole()
{
  if (ArchInterrupts::testIFSet())
  {
    for(;;)
      main_console->getActiveTerminal()->write(nosleep_fifo_->get());
  } 
  else if (main_console->areLocksFree() && main_console->getActiveTerminal()->isLockFree())
  {
    for(uint32 c=0;c<nosleep_fifo_->countElementsAhead();++c)
      main_console->getActiveTerminal()->write(nosleep_fifo_->get());    
  }
}


void oh_writeCharDebugNoSleep(char c)
{
  //this blocks
  writeChar2Bochs((uint8) c);
}
void oh_writeStringDebugNoSleep(char const* str)
{
  //this blocks
  while (*str)
  {
    oh_writeCharDebugNoSleep(*str);
    str++;
  }
}


uint8 const ZEROPAD	= 1;		/* pad with zero */
uint8 const SIGN	= 2;		/* unsigned/signed long */
uint8 const PLUS	= 4;		/* show plus */
uint8 const SPACE	= 8;		/* space if plus */
uint8 const LEFT	= 16;		/* left justified */
uint8 const SPECIAL	= 32;		/* 0x */
uint8 const LARGE	= 64;		/* use 'ABCDEF' instead of 'abcdef' */

void output_number(void (*write_char)(char), uint32 num, uint32 base, uint32 size, uint32 precision, uint8 type)
{
	char c;
  char sign,tmp[70];
	const char *digits;
	static const char small_digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
	static const char large_digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	uint32 i;

	digits = (type & LARGE) ? large_digits : small_digits;
	if (type & LEFT)
  {
		type &= ~ZEROPAD;
    size = 0;  //no padding then
  }
	if (base < 2 || base > 36)
		return;
	c = (type & ZEROPAD) ? '0' : ' ';
	sign = 0;
	if (type & SIGN) {
		if (((int32) num) < 0) {
			sign = '-';
			num = - (int32) num;
		} else if (type & PLUS) {
			sign = '+';
		} else if (type & SPACE) {
			sign = ' ';
		}
	}
	i = 0;
	if (num == 0)
		tmp[i++]='0';
	else while (num != 0)
  {
		tmp[i++] = digits[num%base];
    num /= base;
  }
	//size -= precision;
	//~ if (!(type&(ZEROPAD+LEFT))) {
		//~ while(size-- >0) {
      //~ console->write(' ');
    //~ }
	//~ }
	if (sign) {
    tmp[i++] = sign;
  }
	if (type & SPECIAL) {
    precision = 0; //no precision with special for now
		if (base==8) {
        tmp[i++] = '0';
		} else if (base==16) {
        tmp[i++] = digits[33];
        tmp[i++] = '0';
      }
	}

  if (precision > size)
    precision = size;
  
  while (size-- - precision > i)
    (write_char)((char) c);
  
  while (precision-- > i)         
    (write_char)((char) '0');
    
	while (i-- > 0)
    (write_char)((char) tmp[i]);
	//~ while (size-- > 0) {
		//~ if (buf <= end)
			//~ *buf = ' ';
		//~ ++buf;
	//~ }
}

uint32 atoi(const char *&fmt)
{
  uint32 num=0;
  uint32 log=0;
  while (*fmt >= '0' && *fmt <= '9')
  {
    num*= (10*log);
    num+=*fmt - '0';
    ++log;
    ++fmt;
  }
  return num;
}

// simple vkprintf, doesn't know flags yet
// by Bernhard
void vkprintf(void (*write_string)(char const*), void (*write_char)(char), const char *fmt, va_list args)
{  
  while (fmt && *fmt)
  {
    if (*fmt == '%')
    {
      int32 width = 0;
      uint8 flag = 0;
      ++fmt;
      switch (*fmt) 
      {
        case '-':
          flag |= LEFT;
          ++fmt;
          break;
        case '+':
          flag |= PLUS;
          ++fmt;
          break;
        case '0':
          flag |= ZEROPAD;
          ++fmt;
          break;
        default:
          break;
      }
      if (*fmt > '0' && *fmt <= '9')
        width = atoi(fmt);  //this advances *fmt as well
      
      //handle diouxXfeEgGcs
      switch (*fmt)
      {
        case '%':
          write_char(*fmt);
          break;
        
        case 's':
          write_string(va_arg(args,char const*));
          break;
        
        //signed decimal
        case 'd':
          output_number(write_char,(uint32) va_arg(args,int32),10,width, 0, flag | SIGN);
          break;
        
        //we don't do i until I see what it actually should do
        //case 'i':
        //  break;

        //octal
        case 'o':
          output_number(write_char,(uint32) va_arg(args,uint32),8,width, 0, flag | SPECIAL);
          break;

        //unsigned
        case 'u':
          output_number(write_char,(uint32) va_arg(args,uint32),10,width, 0, flag );
          break;

        case 'x':
          output_number(write_char,(uint32) va_arg(args,uint32),16,width, 0, flag | SPECIAL);
          break;

        case 'X':
          output_number(write_char,(uint32) va_arg(args,uint32), 16, width, 0, flag | SPECIAL | LARGE);
          break;
        
        //no floating point yet
        //case 'f':
        //  break;

        //no scientific notation (yet)
        //case 'e':
        //  break;

        //case 'E':
        //  break;

        //no floating point yet
        //case 'g':
        //  break;

        //case 'G':
        //  break;

        //we don't do unicode (yet)
        case 'c':
          write_char((char) va_arg(args,uint32));
          break;

        default:
          //jump over unknown arg
          ++args;
          break;
      }
      
    }
    else
      write_char(*fmt);
    
    ++fmt;
  }
  
}

//------------------------------------------------
/// Standard kprintf. Usable like any other printf. 
/// Outputs Text on the Console. It is intendet for
/// Kernel Debugging andt herefore avoids using "new".
///
/// @return void
/// @param fmt Format String with standard Format Syntax
/// @param args Possible multibple variable for printf as specified in Format String.
///
///
///
void kprintf(const char *fmt, ...)
{
  va_list args;
  
  va_start(args, fmt);
  //check if atomar or not in current context
  if (likely(ArchInterrupts::testIFSet()) || (unlikely(ArchInterrupts::testIFSet() == false) && main_console->areLocksFree() && main_console->getActiveTerminal()->isLockFree()))
    vkprintf(oh_writeStringWithSleep, oh_writeCharWithSleep, fmt, args);
  else
    vkprintf(oh_writeStringNoSleep, oh_writeCharNoSleep, fmt, args);
  va_end(args);
}

//------------------------------------------------
/// kprintfd is a shorthand for kprintf_debug
/// Outputs Text on the Serial Debug Console. It is intendet for
/// Kernel Debugging andt herefore avoids using "new".
///
/// @return void
/// @param fmt Format String with standard Format Syntax
/// @param args Possible multibple variable for printf as specified in Format String.
///
///
///
void kprintfd(const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  //check if atomar or not in current context
  if (likely(ArchInterrupts::testIFSet()))
    vkprintf(oh_writeStringDebugWithSleep, oh_writeCharDebugWithSleep, fmt, args);
  else
    vkprintf(oh_writeStringDebugNoSleep, oh_writeCharDebugNoSleep, fmt, args);
  va_end(args);
}

//make this obsolete with atomarity check
void kprintf_nosleep(const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  //check if atomar or not in current context
  if (unlikely(ArchInterrupts::testIFSet()) || (likely(ArchInterrupts::testIFSet() == false) && main_console->areLocksFree() && main_console->getActiveTerminal()->isLockFree()))
    vkprintf(oh_writeStringWithSleep, oh_writeCharWithSleep, fmt, args);
  else
    vkprintf(oh_writeStringNoSleep, oh_writeCharNoSleep, fmt, args);
  va_end(args);
}
//make this obsolete with atomarity check
void kprintfd_nosleep(const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  //check if atomar or not in current context
  if (unlikely(ArchInterrupts::testIFSet()))
    vkprintf(oh_writeStringDebugWithSleep, oh_writeCharDebugWithSleep, fmt, args);
  else
    vkprintf(oh_writeStringDebugNoSleep, oh_writeCharDebugNoSleep, fmt, args);
  va_end(args);  
}

void kprintf_nosleep_flush()
{
  //can have only one flush operation here, as each one would block
  flushActiveConsole();
}
