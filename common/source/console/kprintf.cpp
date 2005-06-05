//----------------------------------------------------------------------
//   $Id: kprintf.cpp,v 1.6 2005/06/05 07:59:35 nelles Exp $
//----------------------------------------------------------------------
//   $Log: kprintf.cpp,v $
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
#include "ConsoleManager.h"

uint8 const ZEROPAD	= 1;		/* pad with zero */
uint8 const SIGN	= 2;		/* unsigned/signed long */
uint8 const PLUS	= 4;		/* show plus */
uint8 const SPACE	= 8;		/* space if plus */
uint8 const LEFT	= 16;		/* left justified */
uint8 const SPECIAL	= 32;		/* 0x */
uint8 const LARGE	= 64;		/* use 'ABCDEF' instead of 'abcdef' */

void output_number(Console *console, uint32 num, uint32 base, uint32 size, uint32 precision, uint8 type)
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
    console->write((uint8) c);
  
  while (precision-- > i)         
    console->write((uint8) '0');
    
	while (i-- > 0)
    console->write((uint8) tmp[i]);
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
void vkprintf(Console *console, const char *fmt, va_list args)
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
          console->write(*fmt);
          break;
        
        case 's':
          console->writeString(va_arg(args,uint8*));
          break;
        
        //signed decimal
        case 'd':
          output_number(console,(uint32) va_arg(args,int32),10,width, 0, flag | SIGN);
          break;
        
        //we don't do i until I see what it actually should do
        //case 'i':
        //  break;

        //octal
        case 'o':
          output_number(console,(uint32) va_arg(args,uint32),8,width, 0, flag | SPECIAL);
          break;

        //unsigned
        case 'u':
          output_number(console,(uint32) va_arg(args,uint32),10,width, 0, flag );
          break;

        case 'x':
          output_number(console,(uint32) va_arg(args,uint32),16,width, 0, flag | SPECIAL);
          break;

        case 'X':
          output_number(console,(uint32) va_arg(args,uint32), 16, width, 0, flag | SPECIAL | LARGE);
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
          console->write((uint8) va_arg(args,uint32));
          break;

        default:
          //jump over unknown arg
          ++args;
          break;
      }
      
    }
    else
      console->write(*fmt);
    
    ++fmt;
  }
  
}

void kprintf(const char *fmt, ...)
{
  va_list args;

  Console *console = ConsoleManager::instance()->getActiveConsole();
  
  va_start(args, fmt);
  vkprintf(console, fmt, args);
  va_end(args);
}

void kprintf_debug(const char *fmt, ...)
{
  va_list args;

  Console *console = ConsoleManager::instance()->getDebugConsole();
  
  va_start(args, fmt);
  vkprintf(console, fmt, args);
  va_end(args);
}

void kprintfd(const char *fmt, ...)
{
  va_list args;

  Console *console = ConsoleManager::instance()->getDebugConsole();
  
  va_start(args, fmt);
  vkprintf(console, fmt, args);
  va_end(args);
}