#ifndef BITMAP_H__
#define BITMAP_H__

#include "types.h"

uint8 const bits_per_bitmap_atom_ = 8;

class Bitmap
{

public:
  Bitmap (size_t number_of_bits);
  ~Bitmap ();
  void setBit(size_t bit_number);
  bool getBit(size_t bit_number);
  void unsetBit(size_t bit_number);
  size_t getSize() { return size_; }

  /**
   * returns the number of bits set
   * @return the number of bits set
   */
  size_t getNumBitsSet() { return num_bits_set_; }

  /**
   * returns the number of unset bits
   * @return the number of unset bits
   */
  size_t getNumFreeBits() { return size_ - num_bits_set_; }

  /**
   * prints the bitmap using kprintfd
   */
  void bmprint();

  /**
   * sets a whole byte in the bitmap
   * only use if you know what you are doing
   * @param byte_number the number of the byte to set
   * @param byte the byte to set
   */
  void setByte(size_t byte_number, uint8 byte);

  /**
   * returns a whole byte of a given number
   * only use if you know what you are doing
   * @param byte_number the number of the byte to return
   * @return the byte
   */
  uint8 getByte(size_t byte_number);

private:
  size_t size_;
  size_t num_bits_set_;
  uint8 *bitmap_;
};

#endif /* BITMAP_H__ */
