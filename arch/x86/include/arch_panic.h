/**
 * @file arch_panic.h
 *
 */

#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * prints a panic message to the bochs terminal
 * @param mesg the message to print
 *
 */
void arch_panic(uint8 *mesg);

#ifdef __cplusplus
}
#endif
