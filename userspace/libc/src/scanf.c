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
 * CVS Log Info for $RCSfile: scanf.c,v $
 *
 * $Id: scanf.c,v 1.1 2005/09/16 05:00:58 aniederl Exp $
 * $Log$
 */


#include "unistd.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdarg.h"



//----------------------------------------------------------------------
/**
 * Reads input from stdin according to the given format and
 * assigns read values to the given variables.
 * A detailed description of the format is given in the
 * 'Linux Programmer's Manual'.
 *
 * @param format A string containing the input format, followed by an\
 argument list of variables for assignment
 * @return The number of input items assigned, zero indicates that no input\
 items were assigned while input was available, EOF if failure (e.g.\
 end-of-file) occurs before any items have been read
 *
 */
int scanf(const char *format, ...)
{

  return 0;
}


/**
 * Reads the next character from stdin.
 * The read value will be returned as unsigned char cast to an int
 *
 * @return The read character on success, EOF otherwise and errno is set\
 appropriately
 *
 */
int getchar()
{
  char character = 0;
  ssize_t characters_read = read(STDIN_FILENO, (void*) &character, 1);

  if(!characters_read || (characters_read == -1))
    return EOF;

  return (int) character;
}

/**
 * Reads a line from stdin and stores it in the string pointed to by the
 * argument.
 * Reading is terminated by a newline or EOF which is replaced by '\0'.
 *
 * No check for buffer overrun is performed. Therefore it is highly
 * inadvisible to use this function.
 *
 * @param input_string The string where the input is stored
 * @return A pointer to the input_string on success, NULL otherwise
 *
 */
char *gets(char *input_string)
{
  while((*input_string != '\n') && (*input_string != EOF))
  {
    if(read(STDIN_FILENO, (void*) input_string, 1) == -1)
      return NULL;

    ++input_string;
  }

  if((*input_string != '\n') && (*input_string != EOF))
    *input_string = '\0';

  return input_string;
}
