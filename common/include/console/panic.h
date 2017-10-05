#pragma once

#include "types.h"

/**
 * kernel panic function writes the message and the stack trace to the screen and
 * dies after that
 *
 */
void kpanict( uint8 *message );

