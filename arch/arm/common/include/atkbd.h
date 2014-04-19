/**
 * @file atkbd.h
 *
 */

#ifndef _KMI_H_
#define _KMI_H_

#include "types.h"

struct KMI
{
    uint32 cr;
    uint32 stat;
    uint32 data;
    uint32 ir;
};


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
