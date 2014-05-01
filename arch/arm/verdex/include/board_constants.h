/**
 * @file board_constants.h
 *
 */

#ifndef _BOARD_CONSTANTS_H_
#define _BOARD_CONSTANTS_H_

#define SERIAL_BASE 0x86000000
#define SERIAL_FLAG_REGISTER 0x14
#define SERIAL_BUFFER_FULL (1 << 0)

#define PIC_BASE 0x84000004

#define BOARD_LOAD_BASE 0xA0000000

#endif
