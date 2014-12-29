// Projectname: SWEB
// Simple operating system for educational purposes
//
// Copyright (C) 2005  Andreas Niederl
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.



/**
 * CVS Log Info for $RCSfile: printf.c,v $
 *
 * $Id: printf.c,v 1.8 2005/09/21 16:44:46 aniederl Exp $
 * $Log: printf.c,v $
 * Revision 1.7  2005/09/21 03:31:52  aniederl
 * repaired writeNumber function (didn't increase string length)
 *
 * Revision 1.6  2005/09/20 14:16:08  aniederl
 * repaired wrong macro query
 *
 * Revision 1.5  2005/09/16 03:13:58  aniederl
 * fixed error recognition in putchar and puts
 *
 * Revision 1.4  2005/09/14 23:01:04  aniederl
 * added putchar and puts
 *
 * Revision 1.3  2005/09/13 18:37:58  aniederl
 * modified printf for static memory
 *
 * Revision 1.2  2005/09/11 10:22:36  aniederl
 * now freeing the memory of the output string
 *
 * Revision 1.1  2005/09/11 09:46:15  aniederl
 * initial import of printf
 *
 */


#include "unistd.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdarg.h"
#include "string.h"

/**
 * structure containing useful variables describing a string stored in a
 * character array
 *
 */
typedef struct c_string
{
  // start pointer
  char *start;

  // working pointer
  char *ptr;

  // length of the string
  size_t length;

  // actual allocated size for the string
  size_t size;

} c_string;

unsigned char const ZEROPAD	= 1;		/* pad with zero */
unsigned char const SIGN	= 2;		/* unsigned/signed long */
unsigned char const PLUS	= 4;		/* show plus */
unsigned char const SPACE	= 8;		/* space if plus */
unsigned char const LEFT	= 16;		/* left justified */
unsigned char const SPECIAL	= 32;		/* 0x */
unsigned char const LARGE	= 64;		/* use 'ABCDEF' instead of 'abcdef' */

//----------------------------------------------------------------------
/**
 * Resizes the string in the given c_string structure..
 *
 * @param str structure containing information for the string to resize
 * @param new_size the value for resizing the string
 *
 */
#ifdef STATIC_MEMORY__
void resizeString(c_string *str, unsigned int new_size)
{
  c_string old_string;
  unsigned int copy_count = 0;

  old_string.start = str->start;
  old_string.ptr = str->start;
  old_string.size = str->size;

  str->size = new_size;
  str->start = str->ptr = (char *) malloc(str->size * sizeof(char));

  if(new_size > old_string.size)
    new_size = old_string.size;

  for(; copy_count < new_size; ++copy_count)
    *str->ptr++ = *old_string.ptr++;

  free(old_string.start);
}
#endif // STATIC_MEMORY__

//----------------------------------------------------------------------

/**
 * Writes a number of fill chars into a string
 */
void writeFillChars(c_string *output_string, int size, char fill )
{
  while( size-- > 0 )
  {
    *output_string->ptr++ = fill;
    ++output_string->length;
  }
}

/**
 * Writes a number into a string using the given parameters.
 *
 * @param output_string Structure containing information for the output string
 * @param number The number for writing
 * @param base The base of the number
 * @param size The size (in digits) for output
 * @param precision Precision for output
 * @param type Output type
 *
 */
void writeNumber(c_string *output_string, unsigned int number,
                 unsigned int base, unsigned int size,
                 unsigned int precision, unsigned char type)
{
  // code taken from kprintf's output_number()
	char c;
  char sign,tmp[70];
	const char *digits;
	static const char small_digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
	static const char large_digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	unsigned int i;

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
		if (((int) number) < 0) {
			sign = '-';
			number = - (int) number;
		} else if (type & PLUS) {
			sign = '+';
		} else if (type & SPACE) {
			sign = ' ';
		}
	}
	i = 0;
	if (number == 0)
		tmp[i++]='0';
	else while (number != 0)
  {
		tmp[i++] = digits[number%base];
    number /= base;
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
  {
    *output_string->ptr++ = c;
    ++output_string->length;
  }

  while (precision-- > i)
  {
    *output_string->ptr++ = '0';
    ++output_string->length;
  }

	while (i-- > 0)
  {
    *output_string->ptr++ = tmp[i];
    ++output_string->length;
  }


	//~ while (size-- > 0) {
		//~ if (buf <= end)
			//~ *buf = ' ';
		//~ ++buf;
	//~ }
}

//----------------------------------------------------------------------
/**
 * Writes output to stdout.
 * A detailed description of the format is given in the
 * 'Linux Programmer's Manual'.
 *
 * @param format A string containing the output format, followed by an\
 argument list containing different variables for output
 * @return The number of characters printed or the number of characters that\
 would have been printed if the output was truncated, a negative\
 value is returned on failure
 *
 */
extern int printf(const char *format, ...)
{
// no dynamic memory allocation available yet
#define STATIC_MEMORY__

  c_string output_string;
  int character_count = 256;
  output_string.size = character_count;

#ifdef STATIC_MEMORY__
  char buffer[character_count];

  output_string.start = (char *) &buffer;
#else
  output_string.start = (char *) malloc(output_string.size * sizeof(char));
#endif // STATIC_MEMORY__

  output_string.ptr = output_string.start;
  output_string.length = 0;


  va_list args;

  va_start(args, format);

  // format handling taken from vkprintf
  while (format && *format)
  {
    if(!character_count)
    {
#ifdef STATIC_MEMORY__
      break;
#else
      // resize output string if necessary
      character_count = output_string.size;
      resizeString(&output_string, output_string.size * 2);
#endif // STATIC_MEMORY__
    }

    if (*format == '%')
    {
      int width = 0;
      unsigned char flag = 0;
      ++format;
      switch (*format)
      {
        case '-':
          flag |= LEFT;
          ++format;
          break;
        case '+':
          flag |= PLUS;
          ++format;
          break;
        case '0':
          flag |= ZEROPAD;
          ++format;
          break;
        default:
          break;
      }
      size_t c = 0;
      char num[4];
      for(; *format > '0' && *format <= '9'; ++format, ++c)
        if( c < 4 )
          num[c] = *format;
      num[c < 4 ? c : 3] = 0;
      if( c )
      {
        width = atoi(num); //this advances *format as well

#ifdef STATIC_MEMORY__
        if(character_count < width)
          width = character_count;
#else
        while(character_count < width)
        {
          character_count = output_string.size;
          resizeString(&output_string, output_string.size * 2);
        }
#endif // STATIC_MEMORY__
      }

      //handle diouxXfeEgGcs
      switch (*format)
      {
        case '%':
          *output_string.ptr++ = *format;

          --character_count;
          ++output_string.length;
          break;

        case 's':
        {
          char const *string_arg = va_arg(args, char const*);
          int len = strlen(string_arg);

          // we should align right -> fill with spaces
          if( !((flag & LEFT) && (len < width)) )
          {
            character_count -= width - len;
            if( character_count < 0 )
            {
#ifdef STATIC_MEMORY__
              break;
#else
              // resize output string if necessary
              character_count = output_string.size;
              resizeString(&output_string, output_string.size * 2);
#endif // STATIC_MEMORY__
            }

            writeFillChars(&output_string, width - len, ' ');
          }

          // now print the string
          while(string_arg && *string_arg)
          {
            if(!character_count)
            {
#ifdef STATIC_MEMORY__
              break;
#else
              // resize output string if necessary
              character_count = output_string.size;
              resizeString(&output_string, output_string.size * 2);
#endif // STATIC_MEMORY__
            }


            *output_string.ptr++ = *string_arg++;
            --character_count;
            ++output_string.length;
          }

          // and some fillchars if aligned on the left
          if( flag & LEFT && len < width )
          {
            character_count -= width - len;
            if( character_count < 0 )
            {
#ifdef STATIC_MEMORY__
              break;
#else
              // resize output string if necessary
              character_count = output_string.size;
              resizeString(&output_string, output_string.size * 2);
#endif // STATIC_MEMORY__
            }

            writeFillChars(&output_string, width - len, ' ');
          }

          break;
        }

        //signed decimal
        case 'd':
          writeNumber(&output_string,(unsigned int) va_arg(args,int),10,width, 0, flag | SIGN);
          break;

        //we don't do i until I see what it actually should do
        //case 'i':
        //  break;

        //octal
        case 'o':
          writeNumber(&output_string,(unsigned int) va_arg(args,unsigned int),8,width, 0, flag | SPECIAL);
          break;

        //unsigned
        case 'u':
          writeNumber(&output_string,(unsigned int) va_arg(args,unsigned int),10,width, 0, flag );
          break;

        case 'x':
          writeNumber(&output_string,(unsigned int) va_arg(args,unsigned int),16,width, 0, flag | SPECIAL);
          break;

        case 'X':
          writeNumber(&output_string,(unsigned int) va_arg(args,unsigned int), 16, width, 0, flag | SPECIAL | LARGE);
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
          *output_string.ptr++ = (char) va_arg(args,unsigned int);
          ++output_string.length;
          break;

        default:
          //jump over unknown arg
          //++args;
          va_arg(args,void);
          break;
      }

    }
    else
    {
      *output_string.ptr++ = *format;
      --character_count;
      ++output_string.length;
    }

    ++format;
  }

  va_end(args);

  character_count = write(STDOUT_FILENO,
                          (void*) output_string.start, output_string.length);

#ifndef STATIC_MEMORY__
  free(output_string.start);
#endif // STATIC_MEMORY__

  return (int) character_count;
}


//----------------------------------------------------------------------
/**
 * Writes the given character to stdout.
 * The character is casted to unsigned char.
 *
 * @param character The character for writing
 * @return The character written as unsigned char cast to int or EOF on error
 *
 */
int putchar(int character)
{
  unsigned char output_char = (unsigned char) character;
  int characters_written = write(STDOUT_FILENO, (void*) &output_char, 1);

  if(!characters_written || (characters_written == -1))
    return EOF;

  return (int) output_char;
}

//----------------------------------------------------------------------
/**
 * Writes the given string followed by a newline to stdout.
 *
 * @param output_string The string for writing
 * @return A non-negative number on success or EOF on error
 *
 */
int puts(const char *output_string)
{
  unsigned char newline = '\n';
  const char *string_ptr = output_string;
  size_t string_length = 0;
  int characters_written = 0;

  while(string_ptr && *string_ptr++)
    ++string_length;

  if(string_length)
  {
    characters_written = write(STDOUT_FILENO, (void*) output_string, string_length);
    if(!characters_written || (characters_written == -1))
      return EOF;
  }

  characters_written = write(STDOUT_FILENO, (void*) &newline, 1);

  if(!characters_written || (characters_written == -1))
    return EOF;

  return 0;
}

