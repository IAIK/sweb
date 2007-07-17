#include "Buffer.h"
#include "assert.h"
#include "../../include/console/kprintf.h"

Buffer::Buffer(size_t size)
{
  size_ = size;
  buffer_ = new char[size_*sizeof(char)];
  offset_ = 0;
}

Buffer::Buffer(const Buffer &src)
{
  size_ = src.size_;
  buffer_ = new char[size_*sizeof(char)];
  for (size_t index = 0; index < size_; index++)
  {
    buffer_[index] = src.buffer_[index];
  }
  offset_ = src.offset_;
}

Buffer::~Buffer()
{
  delete[] buffer_;
}
  
uint8 Buffer::getByte(size_t index)
{
  return (uint8)buffer_[index + offset_];
}

uint16 Buffer::get2Bytes(size_t index)
{
  uint16 dst = 0;
  dst |= buffer_[index + 1 + offset_];
  dst = dst << 8;
  dst |= (buffer_[index + offset_] & 0xFF);
  return dst;
}

uint32 Buffer::get4Bytes(size_t index)
{
  uint32 dst = 0;
  dst |= get2Bytes(index + 2);
  dst = dst << 16;
  dst |= (get2Bytes(index) & 0xFFFF);
  return dst;
}

uint64 Buffer::get8Bytes(size_t index)
{
  uint64 dst = 0;
  dst |= get4Bytes(index + 4);
  dst = dst << 16;
  dst |= (get4Bytes(index) & 0xFFFFFFFF);
  return dst;
}

void Buffer::setByte(size_t index, uint8 byte)
{
  assert(index + offset_ < size_);
  buffer_[index + offset_] = (char)byte;
}

void Buffer::set2Bytes(size_t index, uint16 byte)
{
  assert(index+1 + offset_ < size_);
  char first_byte = byte >> 8;
  char second_byte = byte && 0xFF;
  buffer_[index + offset_] = second_byte;
  buffer_[index+1 + offset_] = first_byte;
}

void Buffer::set4Bytes(size_t index, uint32 byte)
{
  assert(index+3  + offset_< size_);
  uint16 first_2_bytes = byte >> 16;
  uint16 second_2_bytes = byte && 0xFFFF;
  set2Bytes(index, second_2_bytes);
  set2Bytes(index+2, first_2_bytes);
}

void Buffer::set8Bytes(size_t index, uint64 byte)
{
  assert(index+7  + offset_< size_);
  uint32 first_4_bytes = byte >> 32;
  uint32 second_4_bytes = byte && 0xFFFFFFFF;
  set2Bytes(index, second_4_bytes);
  set2Bytes(index+4, first_4_bytes);
}

uint32 Buffer::getSize()
{
  return size_ - offset_;
}

char* Buffer::getBuffer()
{
  return buffer_ + offset_;
}

void Buffer::append(Buffer* buffer_to_append)
{
  uint32 new_size = size_ + buffer_to_append->getSize();
  char *new_buffer = new char[new_size];
  size_t index = 0;
  for(; index < this->getSize(); index++)
  {
    new_buffer[index] = buffer_[index];
  }
  for(; index < new_size; index++)
  {
    new_buffer[index] = buffer_to_append->getByte(index);
  }
  delete[] buffer_;
  buffer_ = new_buffer;
}

void Buffer::print()
{
  kprintfd( "\n----Buffer:size:%d------\n",size_);
  for(uint32 i = 0; i<size_;i++)
  {
    kprintfd( "%x",buffer_[i]);
  }
  kprintfd( "\n----Buffer:end------\n");
}
    
void Buffer::clear()
{
  for(uint32 i = 0; i < size_ - offset_; i++)
  {
    buffer_[i + offset_] = 0;
  }
}

void Buffer::setOffset(uint32 offset)
{
  offset_ = offset;
}
