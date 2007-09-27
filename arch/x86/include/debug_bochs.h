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
void writeChar2Bochs( uint8 char2Write );

/**
 * writes a string/line to the bochs terminal
 * max length is 250
 *
 */
void writeLine2Bochs( const uint8 *line2Write );

#endif
