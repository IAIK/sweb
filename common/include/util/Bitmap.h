/**
 * @file Bitmap.h
 */

#ifndef BITMAP_H__
#define BITMAP_H__

#include "types.h"

uint8 const bits_per_bitmap_atom_ = 8;

/**
 * @class Bitmap
 */
class Bitmap
{

  friend class MinixStorageManager;

public:

  /**
   * Constructor
   * @param number_of_bits the size of the bitmap to create
   */
  Bitmap (size_t number_of_bits);

  /**
   * Destructor
   */
  ~Bitmap ();

  /**
   * sets the bit of the given number
   * @param bit_number the bit number
   */
  void setBit(size_t bit_number);

  /**
   * returns the bits value of the given number
   * @param bit_number the bit number
   * @return true if the bit is set
   */
  bool getBit(size_t bit_number);

  /**
   * unsets the bit of the given number
   * @param bit_number the bit number
   */
  void unsetBit(size_t bit_number);

  /**
   * returns the size of the bitmap
   * @return the size
   */
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

protected:
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
  size_t bit_count_;
};

#endif /* BITMAP_H__ */
