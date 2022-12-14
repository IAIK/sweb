#pragma once

#include "types.h"


class ArchSerialInfo
{
public:
  uint16 base_port;  /// The base IO port
  uint8  uart_type;  /// Type of the connected UART
  uint8  irq_num;    /// IRQ number
};

enum class SerialPortRegister : uint32_t
{
    THR = 0, /// UART Transmitter Holding Buffer
    RBR = 0, /// UART Receiver Buffer
    DLL = 0, /// UART Divisor Latch Low Byte
    IER = 1, /// UART Interrupt Enable Register
    DLH = 1, /// UART Divisor Latch High Byte
    IIR = 2, /// UART Interrupt Identification Register
    FCR = 2, /// UART FIFO Control Register
    LCR = 3, /// UART Line Control Register
    MCR = 4, /// UART Modem Control Register
    LSR = 5, /// UART Line Status Register
    MSR = 6, /// UART Modem Status Register
    SR  = 7, /// UART Scratch Register
};

struct SerialPort_InterruptEnableRegister
{
    union
    {
        struct
        {
            uint8_t received_data_available_int_enable       : 1;
            uint8_t transmitter_holding_reg_empty_int_enable : 1;
            uint8_t receiver_line_status_int_enable          : 1;
            uint8_t modem_status_int_enable                  : 1;
            uint8_t sleep_mode                               : 1;
            uint8_t low_power_mode                           : 1;
            uint8_t reserved                                 : 2;
        };
        uint8 u8;
    };
};

static_assert(sizeof(SerialPort_InterruptEnableRegister) == 1);

enum class IIR_int : uint8
{
    MODEM_STATUS = 0b000,
    TRANSMITTER_HOLDING_REG_EMPTY = 0b001,
    RECEIVED_DATA_AVAILABLE = 0b010,
    RECEIVER_LINE_STATUS = 0b011,
    TIMEOUT = 0b110,
};

enum class IIR_fifo_info : uint8
{
    NO_FIFO = 0b00,
    FIFO_NON_FUNCTIONAL = 0b10,
    FIFO_ENABLED = 0b11,
};

struct SerialPort_InterruptIdentificationRegister
{
    union
    {
        struct
        {
            uint8_t int_pending     : 1; // 0 = int pending, 1 = no int pending
            IIR_int int_status      : 3;
            uint8 reserved          : 1;
            uint8 fifo_64b_enabled  : 1;
            IIR_fifo_info fifo_info : 2;
        };

        uint8 u8;
    };
};

static_assert(sizeof(SerialPort_InterruptIdentificationRegister) == 1);

enum class LCR_parity : uint8
{
    NO_PARITY = 0b000,
    ODD_PARITY = 0b001,
    EVEN_PARITY = 0b011,
    MARK = 0b101,
    SPACE = 0b111,
};

enum class LCR_stop_bits : uint8
{
    ONE_STOP_BIT = 0,
    ONEANDHALF_STOP_BITS = 1,
    TWO_STOP_BITS = 1,
};

enum class LCR_word_length : uint8
{
    BITS_5 = 0b00,
    BITS_6 = 0b01,
    BITS_7 = 0b10,
    BITS_8 = 0b11,
};

struct SerialPort_LineControlRegister
{
    union
    {
        struct
        {
            LCR_word_length word_length : 2;
            LCR_stop_bits stop_bits     : 1;
            LCR_parity parity           : 3;
            uint8 break_enable          : 1;
            uint8 divisor_latch         : 1;
        };

        uint8 u8;
    };
};

static_assert(sizeof(SerialPort_LineControlRegister) == 1);

struct SerialPort_LineStatusRegister
{
    union
    {
        struct
        {
            uint8 data_ready                    : 1;
            uint8 overrun_error                 : 1;
            uint8 parity_error                  : 1;
            uint8 framing_error                 : 1;
            uint8 break_interrupt               : 1;
            uint8 empty_transmitter_holding_reg : 1;
            uint8 empty_data_holding_reg        : 1;
            uint8 fifo_receive_error            : 1;
        };

        uint8 u8;
    };
};

static_assert(sizeof(SerialPort_LineStatusRegister) == 1);

enum class FIFO_TRIGGER_LEVEL : uint8
{
    TRIGGER_1_BYTE = 0b00,
    TRIGGER_4_OR_16_BYTES = 0b01,
    TRIGGER_8_OR_32_BYTES = 0b10,
    TRIGGER_16_OR_56_BYTES = 0b11,
};

struct SerialPort_FifoControlRegister
{
    union
    {
        struct
        {
            uint8 enable_fifos               : 1;
            uint8 clear_receive_fifo         : 1;
            uint8 clear_transmit_fifo        : 1;
            uint8 dma_mode_select            : 1;
            uint8 reserved                   : 1;
            uint8 enable_64_byte_fifo        : 1;
            FIFO_TRIGGER_LEVEL trigger_level : 2;
        };

        uint8 u8;
    };
};

static_assert(sizeof(SerialPort_FifoControlRegister) == 1);

struct SerialPort_ModemControlRegister
{
    union
    {
        struct
        {
            uint8 data_terminal_ready : 1;
            uint8 request_to_send     : 1;
            uint8 aux1                : 1;
            uint8 aux2                : 1;
            uint8 loopback            : 1;
            uint8 auto_flow_control   : 1;
            uint8 reserved            : 2;
        };

        uint8 u8;
    };
};

static_assert(sizeof(SerialPort_ModemControlRegister) == 1);

struct SerialPort_ModemStatusRegister
{
    union
    {
        struct
        {
            uint8 delta_clear_to_send          : 1;
            uint8 delta_data_set_ready         : 1;
            uint8 trailing_edge_ring_indicator : 1;
            uint8 delta_data_carrier_detect    : 1;
            uint8 clear_to_send                : 1;
            uint8 data_set_ready               : 1;
            uint8 ring_indicator               : 1;
            uint8 carrier_detect               : 1;
        };

        uint8 u8;
    };
};

static_assert(sizeof(SerialPort_ModemStatusRegister) == 1);

enum class UART_TYPE
{
    /// Old type UART without FIFO buffers
    UART_8250,
    /// Old type UART with FIFO buffers that do not work
    UART_16650,
    /// Most common UART with FIFO buffers
    UART_16650A,
    UART_16750,
};

class SC
{
public:

    /// Maximum serial ports registered in BIOS
    static constexpr uint32 MAX_ARCH_PORTS = 4;
};
