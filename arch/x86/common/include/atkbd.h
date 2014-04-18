/**
 * @file atkbd.h
 *
 */

#ifndef _ATKBD_H_
#define _ATKBD_H_

#include "types.h"
#include "ports.h"
#define ATKBD_DATA 0x60
#define ATKBD_CTRL 0x64

#define LIGHT_ALL 0x7
#define LIGHT_NUM 0x2
#define LIGHT_SCROLL 0x1
#define LIGHT_CAPS 0x4


/**
 * something to get?
 *
 */
bool kbdBufferFull();

/**
 * get scancode
 *
 */
uint8 kbdGetScancode();

/**
 * reset
 *
 */
void kbdReset();

/**
 * sets Numlock
 *
 */
void kbdSetNumlock(bool on);

/**
 * sets Capslock
 *
 */
void kbdSetCapslock(bool on);

/**
 * sets scrolllock
 *
 */
void kbdSetScrolllock(bool on);

#endif
