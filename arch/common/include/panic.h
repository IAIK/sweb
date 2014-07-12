/**
 * @file panic.h
 *
 */

#ifndef __PANIC__H__
#define __PANIC__H__

#include "types.h"

/**
 * kernel panic function writes the message and the stack trace to the screen and
 * dies after that
 *
 */
void kpanict( uint8 *message );

#endif
