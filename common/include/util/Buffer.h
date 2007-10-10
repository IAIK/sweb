/**
 * @file Buffer.h
 */

#ifndef BUFFER_H__
#define BUFFER_H__
#include "types.h"

/**
 * @class Buffer
 * This Buffer helds the data in big endian format and has methods for little endian access
 * It is used to store data from the hard drive which are stored in big endian format
 */
class Buffer
{

  public:
    /**
     * constuctor
     * @param size the size in bytes
     */
    Buffer ( size_t size );

    /**
     * copy constructor
     * @param src the buffer to copy
     */
    Buffer ( const Buffer &src );

    /**
     * destructor
     */
    ~Buffer();

    /**
     * get one Byte at the given index
     * @param index the index of the byte to read
     * @return one byte
     */
    uint8 getByte ( size_t index );

    /**
     * get two Bytes starting at the given index in little endian format
     * @param index the index of the first byte to read
     * @return two bytes
     */
    uint16 get2Bytes ( size_t index );

    /**
     * get four Bytes starting at the given index in little endian format
     * @param index the index of the first byte to read
     * @return four bytes
     */
    uint32 get4Bytes ( size_t index );

    /**
     * get eight Bytes starting at the given index in little endian format
     * @param index the index of the first byte to read
     * @return eight bytes
     */
    uint64 get8Bytes ( size_t index );

    /**
     * set one Byte at the given index
     * @param index the index of the byte to write
     */
    void setByte ( size_t index, uint8 byte );

    /**
     * set two Bytes starting at the given index in big endian format
     * @param index the index of the first byte to write
     */
    void set2Bytes ( size_t index, uint16 byte );

    /**
     * set four Bytes starting at the given index in big endian format
     * @param index the index of the first byte to write
     */
    void set4Bytes ( size_t index, uint32 byte );

    /**
     * set eight Bytes starting at the given index in big endian format
     * @param index the index of the first byte to write
     */
    void set8Bytes ( size_t index, uint64 byte );

    /**
     * get the buffer's size
     * @return the size
     */
    uint32 getSize();

    /**
     * get the buffer as char*
     * @return the char* buffer
     */
    char *getBuffer();

    /**
     * sets all values to zero
     */
    void clear();

    /**
     * prints the buffer to the command line
     */
    void print();

    /**
     * appends a buffer at the end
     * @param buffer_to_append the buffer to append
     */
    void append ( Buffer* buffer_to_append );

    /**
     * set a offset to the buffer
     * this offset exists until it is set to 0 again
     * all access methods to the buffer will use this offset exept clear
     * @param offset the offset
     */
    void setOffset ( uint32 offset );

  private:

    size_t size_;
    char *buffer_;
    uint32 offset_;

};

#endif // BUFFER_H__
