#pragma once

#include "types.h"

#include "EASTL/bit.h"

/**
 * reads 1 byte from the selected I/O port
 * @param port the I/O port number which is read
 *
 */
[[gnu::always_inline]] static inline uint8 inportb(uint16 port)
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
[[gnu::always_inline]] static inline uint8 inportbp(uint16 port)
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
[[gnu::always_inline]] static inline uint16 inportw(uint16 port)
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
static inline uint16 inportwp(uint16 port)
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
[[gnu::always_inline]] static inline void outportb(uint16 port, uint8 val)
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
[[gnu::always_inline]] static inline void outportbp(uint16 port, uint8 value)
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
[[gnu::always_inline]] static inline void outportw(uint16 port, uint16 value)
{
    asm volatile ("outw %w0, %1" : : "a" (value), "id" (port));
}

/**
 * sends 1 word of data to the specified I/O port with slowdown
 * @param port the I/O port number to send data to
 * @param val data value sent to I/O port
 */
[[gnu::always_inline]] static inline void outportwp(uint16 port, uint16 value)
{
  asm volatile ("outw %w0, %1" : : "a" (value), "id" (port));
  asm volatile ("outb %al,$0x80");
}

namespace IoPort
{
    template<class... T> constexpr bool always_false = false;

    template<uint16_t port_, typename T, bool readable_ = true, bool writeable_ = true>
    struct IoPortDescription
    {
        using value_type = T;
        using port = eastl::integral_constant<uint16_t, port_>;
        using readable = eastl::bool_constant<readable_>;
        using writeable = eastl::bool_constant<writeable_>;
    };

    template<typename T>
    concept io_port_description = requires(T x)
    {
        typename T::value_type;
        typename T::port;
        typename T::readable;
        typename T::writeable;
    };

    template<typename T>
    [[gnu::always_inline]] static inline void write(uint16_t port, const T& v)
    {
        if constexpr (sizeof(T) == 1)
        {
            outportb(port, eastl::bit_cast<uint8_t>(v));
        }
        else if constexpr (sizeof(T) == 2)
        {
            outportw(port, eastl::bit_cast<uint16_t>(v));
        }
        else
        {
            static_assert(always_false<T>, "Invalid size for I/O port write");
        }
    }

    template<typename T>
    [[gnu::always_inline]] static inline T read(uint16_t port)
    {
        if constexpr (sizeof(T) == 1)
        {
            return eastl::bit_cast<T>(inportb(port));
        }
        else if constexpr (sizeof(T) == 2)
        {
            return eastl::bit_cast<T>(inportw(port));
        }
        else
        {
            static_assert(always_false<T>, "Invalid size for I/O port read");
        }
    }

    template<typename PD>
    requires(PD::writeable::value)
    [[gnu::always_inline]] static inline void write(const typename PD::value_type& v)
    {
        write<PD::value_type>(PD::port, v);
    }

    template<typename PD>
    requires(PD::readable::value)
    [[gnu::always_inline]] static inline typename PD::value_type read()
    {
        return read<PD::value_type>(PD::port);
    }

    template<uint16_t port, typename T, bool readable, bool writeable>
    struct StaticIoRegister
    {
        [[gnu::always_inline]] static inline void write(const T& v) requires(writeable)
        {
            IoPort::write<T>(port, v);
        }

        [[gnu::always_inline]] static inline T read() requires(writeable)
        {
            return IoPort::read<T>(port);
        }
    };

    template<io_port_description PD>
    using StaticIoRegister_ = StaticIoRegister<PD::port::value, typename PD::value_type, PD::readable::value, PD::writeable::value>;

    template<typename T, bool readable, bool writeable>
    struct IoRegister
    {
        const uint16_t port;

        inline void write(const T& v) requires(writeable) { IoPort::write<T>(port, v); }

        inline T read() requires(readable) { return IoPort::read<T>(port); }
    };

    struct IoRegisterSet
    {
        const uint16_t base_port;

        template<typename PD>
        requires(PD::writeable::value)
        inline void write(const typename PD::value_type& v)
        {
            IoPort::write<PD::value_type>(base_port + PD::port::value, v);
        }

        template<typename PD>
        requires(PD::readable::value)
        inline typename PD::value_type read()
        {
            return IoPort::read<PD::value_type>(base_port + PD::port::value);
        }

        template<typename PD>
        inline IoRegister<typename PD::value_type, PD::readable::value, PD::writeable::value>
        operator[](PD)
        {
            return {static_cast<uint16_t>(base_port + PD::port::value)};
        }
    };
};

static constexpr uint16_t EMULATOR_DEBUGCONSOLE_PORTNUM = 0xE9;

using EMULATOR_DEBUGCONSOLE =
    IoPort::StaticIoRegister<EMULATOR_DEBUGCONSOLE_PORTNUM, uint8_t, false, true>;

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

using IMCR_SELECT =
    IoPort::StaticIoRegister<(uint16_t)ICMRIoPorts::IMCR_SELECT, IMCRSelect, false, true>;
using IMCR_DATA =
    IoPort::StaticIoRegister<(uint16_t)ICMRIoPorts::IMCR_DATA, IMCRData, false, true>;
