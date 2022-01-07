#pragma once

#include "types.h"

#define PIT_PORT_CH_0_DATA 0x40
#define PIT_PORT_COMMAND   0x43

class PIT
{
public:
        struct PITCommandRegister
        {
                union
                {
                        uint8 value;
                        struct
                        {
                                uint8 bcd_mode       : 1;
                                uint8 operating_mode : 3;
                                uint8 access_mode    : 2;
                                uint8 channel : 2;
                        }__attribute__((packed));
                };
        }__attribute__((packed));

        static void init(uint8 command, uint16 divisor);

        static void sendCommand(uint8 command);

        static void setFrequencyDivisor(uint16 reload_value);
private:
};
