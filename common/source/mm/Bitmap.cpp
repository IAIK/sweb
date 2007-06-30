/**
 * $Id: Bitmap.cpp,v 1.1 2005/04/22 17:33:43 btittelbach Exp $
 *
 * $Log: Bitmap.cpp,v $
 * Revision 1.1  2005/04/22 02:39:16  btittelbach
 *
 *
*/

#include "../../include/mm/Bitmap.h"
#include "assert.h"

Bitmap::Bitmap (size_t number_of_bits)
{
  size_ = number_of_bits;
  size_t byte_count = number_of_bits / bits_per_bitmap_atom_ 
               + ((number_of_bits % bits_per_bitmap_atom_ > 0)?1:0);
  bit_count_ = byte_count * bits_per_bitmap_atom_;
  bitmap_=new uint8[byte_count];
  for (size_t byte = 0; byte < byte_count; ++byte)
    *(bitmap_+byte) = static_cast<uint8>(0);
}

Bitmap::~Bitmap ()
{
  delete bitmap_;
}

void Bitmap::setBit(size_t bit_number)
{
  assert(bit_number < size_);
  size_t byte_number = bit_number / bits_per_bitmap_atom_;
  size_t bit_offset = bit_number % bits_per_bitmap_atom_;
  *(bitmap_+byte_number) |= (1 << bit_offset);
}
bool Bitmap::getBit(size_t bit_number)
{
  assert(bit_number < size_);
  size_t byte_number = bit_number / bits_per_bitmap_atom_;
  size_t bit_offset = bit_number % bits_per_bitmap_atom_;
  return (*(bitmap_+byte_number) & (1 << bit_offset));

}
