/**
 * @file debug_bochs.h
 *
 */

#ifndef _DEBUG_BOCHS_H_
#define _DEBUG_BOCHS_H_
#include "types.h"

/**
 * writes a char to the bochs terminal
 *
 */
void writeChar2Bochs( char char2Write );

/**
 * writes a string/line to the bochs terminal
 * max length is 250
 *
 */
void writeLine2Bochs( const char *line2Write );

#endif
