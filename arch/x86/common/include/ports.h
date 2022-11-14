#pragma once

#include "types.h"
#include "EASTL/bit.h"


/**
 * reads 1 byte from the selected I/O port
 * @param port the I/O port number which is read
 *
 */
static inline uint8 inportb(uint16 port)
{
   uint8 return_val;
   __asm__ __volatile__ (
   "inb %1,%0"
   : "=a"(return_val)
   : "d"(port));

   return return_val;
}

/**
 * reads 1 byte from the selected I/O port with slowdown
 * @param port the I/O port number which is read
 *
 */
static inline uint8 inportbp(uint16 port)
{
  uint8 _v;
  asm volatile ("inb %1, %0" : "=a" (_v) : "id" (port));
  asm volatile ("outb %al,$0x80" );
  return _v;
}

/**
 * reads 1 word from the selected I/O port
 * @param port the I/O port number which is read
 *
 */
static inline uint16 inportw(uint16 port)
{
  uint16 _res;
  asm volatile ("inw %1, %0" : "=a" (_res) : "id" (port));
  return _res;
}

/**
 * reads 1 word from the selected I/O port with slowdown
 * @param port the I/O port number which is read
 *
 */
__attribute__((unused))
static uint16 inportwp(uint16 port)
{
  uint16 _res;
  asm volatile ("inw %1, %0" : "=a" (_res) : "id" (port));
  asm volatile ("outb %al,$0x80");
  return _res;
}

/**
 * sends 1 byte of data to the specified I/O port
 * @param port the I/O port number to send data to
 * @param val data value sent to I/O port
 *
 */
static inline void outportb(uint16 port, uint8 val)
{
   __asm__ __volatile__ (
   "outb %b0, %w1\n"
   :
   : "a"(val), "d"(port));
}

/**
 * sends 1 byte of data to the specified I/O port with slowdown
 * @param port the I/O port number to send data to
 * @param val data value sent to I/O port
 */
static inline void outportbp(uint16 port, uint8 value)
{
  asm volatile ("outb %b0, %1" : : "a" (value), "id" (port));
  asm volatile ("outb %al,$0x80");
}

/**
 * sends 1 word of data to the specified I/O port
 * @param port the I/O port number to send data to
 * @param val data value sent to I/O port
 *
 */
static inline void outportw(uint16 port, uint16 value)
{
    asm volatile ("outw %w0, %1" : : "a" (value), "id" (port));
}

/**
 * sends 1 word of data to the specified I/O port with slowdown
 * @param port the I/O port number to send data to
 * @param val data value sent to I/O port
 */
static inline void outportwp(uint16 port, uint16 value)
{
  asm volatile ("outw %w0, %1" : : "a" (value), "id" (port));
  asm volatile ("outb %al,$0x80");
}

template<uint16_t port_, typename T, bool readable_ = true, bool writeable_ = true>
struct IoRegister
{
    using value_type = T;

    static constexpr uint16_t port = port_;
    static constexpr bool readable = readable_;
    static constexpr bool writeable = writeable_;

    static_assert(sizeof(value_type) == sizeof(uint8_t) || sizeof(value_type) == sizeof(uint16_t));

    template <uint32_t _port = port, bool _writeable = writeable>
    static eastl::enable_if_t<_writeable, void> write(value_type v)
    {
        if constexpr (sizeof(value_type) == 1)
        {
            outportb(port, eastl::bit_cast<uint8_t>(v));
        }
        else if constexpr (sizeof(value_type) == 2)
        {
            outportw(port, eastl::bit_cast<uint16_t>(v));
        }
    }

    template <uint32_t _port = port, bool _readable = readable>
    static eastl::enable_if_t<_readable, value_type> read()
    {
        if constexpr (sizeof(value_type) == 1)
        {
            return eastl::bit_cast<value_type>(inportb(port));
        }
        else if constexpr (sizeof(value_type) == 2)
        {
            return eastl::bit_cast<value_type>(inportw(port));
        }
    }
};


enum class ICMRIoPorts : uint16_t
{
    IMCR_SELECT = 0x22,
    IMCR_DATA = 0x23,
};

enum class IMCRSelect : uint8_t
{
    SELECT_IMCR = 0x70,
};

enum class IMCRData : uint8_t
{
    PIC_MODE = 0x0,
    APIC_PASSTHROUGH = 0x1,
};

using IMCR_SELECT = IoRegister<(uint16_t)ICMRIoPorts::IMCR_SELECT, IMCRSelect, false, true>;
using IMCR_DATA = IoRegister<(uint16_t)ICMRIoPorts::IMCR_DATA, IMCRData, false, true>;
