#pragma once

namespace ATACommand
{
    namespace Other
    {
        enum
        {
            NOP = 0x00,
            SET_MULTIPLE_MODE = 0xC6,
            FLUSH_CACHE = 0xE7,
            SET_FEATURES = 0xEF,
        };
    };

    namespace PIO
    {
        enum
        {
            READ_SECTORS = 0x20,
            WRITE_SECTORS = 0x30,
            READ_MULTIPLE = 0xC4,
            IDENTIFY_DEVICE = 0xEC,
        };
    };

    namespace DMA
    {
        enum
        {
            READ = 0xC8,
            WRITE = 0xCA,
        };
    };

    // 48-bit commands
    namespace EXT
    {
        namespace Other
        {
            enum
            {
                DATA_SET_MANAGEMENT = 0x06,
                CONFIGURE_STREAM = 0x51,
                SET_DATE_TIME = 0x77,
            };
        };

        namespace PIO
        {
            enum
            {
                READ_SECTORS = 0x24,
                READ_MULTIPLE = 0x29,
                READ_STREAM = 0x2B,
                READ_LOG = 0x2F,
            };
        };

        namespace DMA
        {
            enum
            {
                READ = 0x25,
                WRITE = 0x35,
                READ_LOG = 0x47,
                READ_STREAM = 0x2A,
            };
        };
    };
}; // namespace ATACommand
