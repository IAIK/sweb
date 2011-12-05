/**
 * @file Buffer.cpp
 */

#include "Buffer.h"
#include "assert.h"
#include "console/kprintf.h"
#include "ArchCommon.h"

Buffer::Buffer ( size_t size )
{
  size_ = size;
  buffer_ = new char[size_];
  offset_ = 0;
}

Buffer::Buffer ( const Buffer &src )
{
  size_ = src.size_;
  buffer_ = new char[size_];
  for ( size_t index = 0; index < size_; index++ )
  {
    buffer_[index] = src.buffer_[index];
  }
  offset_ = src.offset_;
}

Buffer::~Buffer()
{
  delete[] buffer_;
}

void Buffer::memcpy(size_t offset, const char* src, size_t n)
{
  ArchCommon::memcpy((pointer) (buffer_ + offset_ + offset), (pointer) src, n);
}


uint8 Buffer::getByte ( size_t index )
{
  return ( uint8 ) buffer_[index + offset_];
}

uint16 Buffer::get2Bytes ( size_t index )
{
  return *(( uint16* ) (buffer_ + index + offset_));
}

uint32 Buffer::get4Bytes ( size_t index )
{
  return *(( uint32* ) (buffer_ + index + offset_));
}

uint64 Buffer::get8Bytes ( size_t index )
{
  return *(( uint64* ) (buffer_ + index + offset_));
}

void Buffer::setByte ( size_t index, uint8 byte )
{
  //assert ( index + offset_ < size_ );
  buffer_[index + offset_] = ( char ) byte;
}

void Buffer::set2Bytes ( size_t index, uint16 byte )
{
  //assert ( index + 1 + offset_ < size_ );
  *((uint16*) (buffer_ + index + offset_)) = byte;
}

void Buffer::set4Bytes ( size_t index, uint32 byte )
{
  //assert ( index+3  + offset_< size_ );
  *((uint32*) (buffer_ + index + offset_)) = byte;
}

void Buffer::set8Bytes ( size_t index, uint64 byte )
{
  //assert ( index+7  + offset_< size_ );
  *((uint64*) (buffer_ + index + offset_)) = byte;
}

uint32 Buffer::getSize()
{
  return size_ - offset_;
}

char* Buffer::getBuffer()
{
  return buffer_ + offset_;
}

void Buffer::append ( Buffer* buffer_to_append )
{
  uint32 new_size = size_ + buffer_to_append->getSize();
  char *new_buffer = new char[new_size];
  size_t index = 0;
  for ( ; index < this->getSize(); index++ )
  {
    new_buffer[index] = buffer_[index];
  }
  for ( ; index < new_size; index++ )
  {
    new_buffer[index] = buffer_to_append->getByte ( index );
  }
  delete[] buffer_;
  buffer_ = new_buffer;
}

void Buffer::print()
{
  kprintfd ( "\n----Buffer:size:%d-offset:%d-----\n",size_,offset_ );
  for ( uint32 i = 0; i<size_;i++ )
  {
    kprintfd ( "%x",buffer_[i] );
  }
  kprintfd ( "\n----Buffer:end------\n" );
}

void Buffer::clear()
{
  for ( uint32 i = 0; i < size_ - offset_; i++ )
  {
    buffer_[i + offset_] = 0;
  }
}

void Buffer::setOffset ( uint32 offset )
{
  offset_ = offset;
}
