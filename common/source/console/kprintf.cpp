//----------------------------------------------------------------------
//   $Id: kprintf.cpp,v 1.2 2005/04/24 18:58:45 btittelbach Exp $
//----------------------------------------------------------------------
//   $Log: kprintf.cpp,v $
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

void output_number(Console *console, uint32 num, uint32 base, int32 precision, uint8 type)
{
	//char c
  char sign,tmp[66];
	const char *digits;
	static const char small_digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
	static const char large_digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int32 i;

	digits = (type & LARGE) ? large_digits : small_digits;
	//~ if (type & LEFT)
		//~ type &= ~ZEROPAD;
	if (base < 2 || base > 36)
		return;
	//~ c = (type & ZEROPAD) ? '0' : ' ';
	sign = 0;
	if (type & SIGN) {
		if (((int32) num) < 0) {
			sign = '-';
			num = - (int32) num;
			//size--;
		} else if (type & PLUS) {
			sign = '+';
			//size--;
		} else if (type & SPACE) {
			sign = ' ';
			//size--;
		}
	}
	//~ if (type & SPECIAL) {
		//~ if (base == 16)
			//~ size -= 2;
		//~ else if (base == 8)
			//~ size--;
	//~ }
	i = 0;
	if (num == 0)
		tmp[i++]='0';
	else while (num != 0)
  {
		tmp[i++] = digits[num%base];
    num /= base;
  }
	if (i > precision)
		precision = i;
	//size -= precision;
	//~ if (!(type&(ZEROPAD+LEFT))) {
		//~ while(size-- >0) {
      //~ console->write(' ');
    //~ }
	//~ }
	if (sign) {
    console->write((uint8) sign);
  }
	if (type & SPECIAL) {
		if (base==8) {
        console->write((uint8) '0');
		} else if (base==16) {
        console->write((uint8) '0');
        console->write((uint8) 'x');  //small x regardless of digit caps
      }
	}
	//~ if (!(type & LEFT)) {
		//~ while (size-- > 0) {
      //~ console->write((uint8) c); }
	//~ }
	while (i < precision--)         
    console->write((uint8) '0');
	while (i-- > 0)
    console->write((uint8) tmp[i]);
	//~ while (size-- > 0) {
		//~ if (buf <= end)
			//~ *buf = ' ';
		//~ ++buf;
	//~ }
}

// simple vkprintf, doesn't know flags yet
// by Bernhard
void vkprintf(Console *console, const char *fmt, va_list args)
{  
  while (fmt && *fmt)
  {
    if (*fmt == '%')
    {
      ++fmt;
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
          output_number(console,(uint32) va_arg(args,int32),10,0, 0 | SIGN);
          break;
        
        //we don't do i until I see what it actually should do
        //case 'i':
        //  break;

        //octal
        case 'o':
          output_number(console,(uint32) va_arg(args,uint32),8,0, 0 | SPECIAL);
          break;

        //unsigned
        case 'u':
          output_number(console,(uint32) va_arg(args,uint32),10,0, 0 );
          break;

        case 'x':
          output_number(console,(uint32) va_arg(args,uint32),16,0, 0 | SPECIAL);
          break;

        case 'X':
          output_number(console,(uint32) va_arg(args,uint32),16,0, 0 | SPECIAL | LARGE);
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
        //case 'c':
        //  break;

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
