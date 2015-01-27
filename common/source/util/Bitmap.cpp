#include "Bitmap.h"
#include "kprintf.h"
#include "assert.h"

static const uint8 BIT_COUNT[] =
{
  0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3,
  3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4,
  3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4,
  4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5,
  3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 1, 2,
  2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5,
  4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5,
  5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5,
  5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

Bitmap::Bitmap (size_t number_of_bits)
{
  size_ = number_of_bits;
  num_bits_set_ = 0;
  size_t byte_count = number_of_bits / bits_per_bitmap_atom_
               + ((number_of_bits % bits_per_bitmap_atom_ > 0)?1:0);
  bitmap_ = new uint8[byte_count];
  for (size_t byte = 0; byte < byte_count; ++byte)
    *(bitmap_+byte) = static_cast<uint8>(0);
}

Bitmap::~Bitmap ()
{
  delete[] bitmap_;
}

void Bitmap::setBit(size_t bit_number)
{
  assert(bit_number < size_);
  const size_t byte_number = bit_number / bits_per_bitmap_atom_;
  const size_t bit_offset = bit_number % bits_per_bitmap_atom_;
  const uint8 mask = 1 << bit_offset;
  uint8& byte = bitmap_[byte_number];

  if (!(byte & mask))
  {
    byte |= mask;
    ++num_bits_set_;
  }
}

bool Bitmap::getBit(size_t bit_number)
{
  assert(bit_number < size_);
  size_t byte_number = bit_number / bits_per_bitmap_atom_;
  size_t bit_offset = bit_number % bits_per_bitmap_atom_;
  return (*(bitmap_+byte_number) & (1 << bit_offset));
}

void Bitmap::unsetBit(size_t bit_number)
{
  assert(bit_number < size_);
  size_t byte_number = bit_number / bits_per_bitmap_atom_;
  size_t bit_offset = bit_number % bits_per_bitmap_atom_;
  const uint8 mask = 1 << bit_offset;
  uint8& byte = bitmap_[byte_number];

  if (byte & mask)
  {
    byte &= ~(1 << bit_offset);
    --num_bits_set_;
  }
}

void Bitmap::setByte(size_t byte_number, uint8 byte)
{
  assert(byte_number*bits_per_bitmap_atom_ < size_);
  uint8& b = bitmap_[byte_number];

  num_bits_set_ -= BIT_COUNT[b];
  num_bits_set_ += BIT_COUNT[byte];
  b = byte;
}

uint8 Bitmap::getByte(size_t byte_number)
{
  assert(byte_number*bits_per_bitmap_atom_ < size_);
  return bitmap_[byte_number];
}

void Bitmap::bmprint()
{
  kprintfd("\n-----Bitmap: size=%ld, num_bits_set=%ld-----\n",size_,num_bits_set_);
  for(uint32 i = 0; i < size_; i++)
  {
    kprintfd( "%d", getBit( i));
  }
  kprintfd("\n-----Bitmap:end------\n");
}
