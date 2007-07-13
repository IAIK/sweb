#ifndef BUFFER_H__
#define BUFFER_H__
#include "types.h"

class Buffer
{

  public:
    Buffer(size_t size);
    Buffer(const Buffer &src);
    ~Buffer();
  
    uint8 getByte(size_t index);
    uint16 get2Bytes(size_t index);
    uint32 get4Bytes(size_t index);
    uint64 get8Bytes(size_t index);
  
    void setByte(size_t index, uint8 byte);
    void set2Bytes(size_t index, uint16 byte);
    void set4Bytes(size_t index, uint32 byte);
    void set8Bytes(size_t index, uint64 byte);
  
    uint32 getSize();
    char *getBuffer();
    void clear();
    
    void print();
  private:

    size_t size_;
    char *buffer_;

};

#endif // BUFFER_H__
