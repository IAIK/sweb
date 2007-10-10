/**
 * @file kprintf.h
 */

#include "stdarg.h"
#include "types.h"
#include "debug.h"

/**
 * Standard kprintf. Usable like any other printf.
 * Outputs Text on the Console. It is intendet for
 * Kernel Debugging and therefore avoids using "new".
 * @param fmt Format String with standard Format Syntax
 * @param args Possible multibple variable for printf as specified in Format String.
 */
void kprintf ( const char *fmt, ... );

/**
 * Usable like any other printf.
 * kprintfd is a shorthand for kprintf_debug
 * Outputs Text on the Serial Debug Console. It is intendet for
 * Kernel Debugging and therefore avoids using "new".
 * @param fmt Format String with standard Format Syntax
 * @param args Possible multibple variable for printf as specified in Format String.
 */
void kprintfd ( const char *fmt, ... );

/**
 * Usable like any other printf.
 * Outputs Text on the Console. It can safely be used even if
 * interrupts are turned off, but doesn't guarantee that it prints at all.
 * @param fmt Format String with standard Format Syntax
 * @param args Possible multibple variable for printf as specified in Format String.
 */
void kprintf_nosleep ( const char *fmt, ... );

/**
 * Usable like any other printf.
 * Outputs Text on the Serial Debug Console. It can safely be used even if
 * interrupts are turned off, but doesn't guarantee that it prints at all.
 * @param fmt Format String with standard Format Syntax
 * @param args Possible multibple variable for printf as specified in Format String.
 */
void kprintfd_nosleep ( const char *fmt, ... );
/**
 * prints out a whole buffer with the given size to the Console.
 * @param buffer the buffer to print
 * @param size the buffer's size
 */
void kprint_buffer ( char *buffer, uint32 size );

/**
 * Initializes the nosleep functionality and starts the flushing thread.
 */
void kprintf_nosleep_init();

/**
 * Displays the output only is the debug flag is set in debug.h
 * needs a debug flag as first parameter otherwise usable like any other printf
 * adds an additional prompt depending on the debug flag
 * please add your custom debug flag here if needed
 * @param flag the debug flag
 * @param fmt Format String with standard Format Syntax
 * @param args Possible multibple variable for printf as specified in Format String.
 */
void debug ( uint32 flag, const char *fmt, ... );

/**
 * returns true if the Debug output for the given flag is enabled
 * @param flag the flag to check
 * @return true if debug is enabled
 */
bool isDebugEnabled ( uint32 flage );

