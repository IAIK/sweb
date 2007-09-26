/**
 * @file image.h
 */

#ifndef __IMAGE__H_
#define __IMAGE__H_

#include "types.h"

struct sweb_logo
{
  uint32   width;
  uint32   height;
  uint32   bytes_per_pixel; /* 3:RGB, 4:RGBA */
  int8   *pixel_data;
} ;

extern const struct sweb_logo logo;

#endif
