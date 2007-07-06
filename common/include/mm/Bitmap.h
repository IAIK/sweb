/***************************************************************************
 * $Id: Bitmap.h,v 1.3 2005/05/03 17:13:08 nomenquis Exp $
 *
 *  Fri Apr 22 04:02:06 2005
 *  Copyright  2005  Bernhard Tittelbach
 *
 *  In case someone should need a bitmap
 *
 * $Log: Bitmap.h,v $
 * Revision 1.2  2005/05/02 19:58:40  nelles
 * made GetStackPointer in Thread public
 * added panic.cpp
 *
 * Revision 1.1  2005/04/22 17:32:25  btittelbach
 * Renamed Bitmap per Request
 *
 *
 *
 ****************************************************************************/

#ifndef _BITMAP_H
#define _BITMAP_H

#include "types.h"

uint8 const bits_per_bitmap_atom_ = 8;

class Bitmap
{

  friend class MinixStorageManager;
  
public:
  Bitmap (size_t number_of_bits);
  ~Bitmap ();
  void setBit(size_t bit_number);
  bool getBit(size_t bit_number);
  void unsetBit(size_t bit_number);
  size_t getSize() { return size_; }
  size_t getNumBitsSet() { return num_bits_set_; }
  size_t getNumFreeBits() { return size_ - num_bits_set_; }

protected:
  void setByte(size_t byte_number, uint8 byte);

private:
  size_t size_;
  size_t num_bits_set_;
  uint8 *bitmap_;
  size_t bit_count_;
};

#endif /* _BITMAP_H */
